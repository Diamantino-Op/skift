#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

//
#include <karm-io/funcs.h>
#include <karm-logger/logger.h>

#include <karm-sys/_embed.h>

#include "fd.h"
#include "utils.h"

namespace Karm::Sys::_Embed {

Res<Mime::Path> resolve(Mime::Url const &url) {
    Mime::Path resolved;
    if (url.scheme == "file") {
        resolved = url.path;
    } else if (url.scheme == "ipc") {
        auto const *runtimeDir = getenv("XDG_RUNTIME_DIR");
        if (not runtimeDir) {
            runtimeDir = "/tmp/";
            logWarn("XDG_RUNTIME_DIR not set, falling back on {}", runtimeDir);
        }

        auto path = url.path;
        path.rooted = false;

        resolved = Mime::Path::parse(runtimeDir).join(path);
    } else if (url.scheme == "bundle") {
        auto *maybeRepo = getenv("CK_BUILDDIR");

        if (not maybeRepo)
            maybeRepo = getenv("SKIFT_BUNDLES");

        if (not maybeRepo)
            return Error::notFound("SKIFT_BUNDLES not set");

        auto path = url.path;
        path.rooted = false;

        resolved = Mime::Path::parse(maybeRepo)
                       .join(url.host)
                       .join("__res__")
                       .join(path);
    } else if (url.scheme == "location") {
        auto *maybeHome = getenv("HOME");
        if (not maybeHome)
            return Error::notFound("HOME not set");

        auto path = url.path;
        path.rooted = false;

        if (url.host == "home")
            resolved = Mime::Path::parse(maybeHome).join(path);
        else
            resolved = Mime::Path::parse(maybeHome).join(Io::toPascalCase(url.host).unwrap()).join(path);
    } else {
        return Error::notFound("unknown url scheme");
    }

    return Ok(resolved);
}

Res<Strong<Sys::Fd>> openFile(Mime::Url const &url) {
    String str = try$(resolve(url)).str();

    isize raw = ::open(str.buf(), O_RDONLY);
    if (raw < 0)
        return Posix::fromLastErrno();
    auto fd = makeStrong<Posix::Fd>(raw);
    if (try$(fd->stat()).type == Stat::DIR)
        return Error::isADirectory();
    return Ok(fd);
}

Res<Strong<Sys::Fd>> createFile(Mime::Url const &url) {
    String str = try$(resolve(url)).str();

    auto raw = ::open(str.buf(), O_RDWR | O_CREAT, 0644);
    if (raw < 0)
        return Posix::fromLastErrno();
    return Ok(makeStrong<Posix::Fd>(raw));
}

Res<Strong<Sys::Fd>> openOrCreateFile(Mime::Url const &url) {
    String str = try$(resolve(url)).str();

    auto raw = ::open(str.buf(), O_RDWR | O_CREAT, 0644);
    if (raw < 0)
        return Posix::fromLastErrno();
    auto fd = makeStrong<Posix::Fd>(raw);
    if (try$(fd->stat()).type == Stat::DIR)
        return Error::isADirectory();
    return Ok(fd);
}

Res<Pair<Strong<Sys::Fd>>> createPipe() {
    int fds[2];

    if (::pipe(fds) < 0)
        return Posix::fromLastErrno();

    return Ok(Pair<Strong<Sys::Fd>>{
        makeStrong<Posix::Fd>(fds[0]),
        makeStrong<Posix::Fd>(fds[1]),
    });
}

Res<Strong<Sys::Fd>> createIn() {
    auto fd = makeStrong<Posix::Fd>(0);
    fd->_leak = true; // Don't close stdin when we close the fd
    return Ok(fd);
}

Res<Strong<Sys::Fd>> createOut() {
    auto fd = makeStrong<Posix::Fd>(1);
    fd->_leak = true; // Don't close stdout when we close the fd
    return Ok(fd);
}

Res<Strong<Sys::Fd>> createErr() {
    auto fd = makeStrong<Posix::Fd>(2);
    fd->_leak = true; // Don't close stderr when we close the fd
    return Ok(fd);
}

Res<Vec<Sys::DirEntry>> readDir(Mime::Url const &url) {
    String str = try$(resolve(url)).str();

    DIR *dir = ::opendir(str.buf());
    if (not dir)
        return Posix::fromLastErrno();

    Vec<Sys::DirEntry> entries;
    struct dirent *entry;
    errno = 0;
    while ((entry = ::readdir(dir))) {
        try$(Posix::consumeErrno());

        if (strcmp(entry->d_name, ".") == 0 or
            strcmp(entry->d_name, "..") == 0 or
            (cstrLen(entry->d_name) >= 1 and entry->d_name[0] == '.')) {
            continue;
        }

        entries.pushBack(Sys::DirEntry{
            Str::fromNullterminated(entry->d_name),
            entry->d_type == DT_DIR,
        });
    }

    if (::closedir(dir) < 0)
        return Posix::fromLastErrno();

    return Ok(entries);
}

Res<Stat> stat(Mime::Url const &url) {
    String str = try$(resolve(url)).str();
    struct stat buf;
    if (::stat(str.buf(), &buf) < 0)
        return Posix::fromLastErrno();
    return Ok(Posix::fromStat(buf));
}

// MARK: Sockets ---------------------------------------------------------------

Res<Strong<Sys::Fd>> listenUdp(SocketAddr addr) {
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return Posix::fromLastErrno();

    struct sockaddr_in addr_ = Posix::toSockAddr(addr);

    if (::bind(fd, (struct sockaddr *)&addr_, sizeof(addr_)) < 0)
        return Posix::fromLastErrno();

    return Ok(makeStrong<Posix::Fd>(fd));
}

Res<Strong<Sys::Fd>> connectTcp(SocketAddr addr) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return Posix::fromLastErrno();

    struct sockaddr_in addr_ = Posix::toSockAddr(addr);
    if (::connect(fd, (struct sockaddr *)&addr_, sizeof(addr_)) < 0)
        return Posix::fromLastErrno();

    return Ok(makeStrong<Posix::Fd>(fd));
}

Res<Strong<Sys::Fd>> listenTcp(SocketAddr addr) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return Posix::fromLastErrno();

    int opt = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        return Posix::fromLastErrno();

    struct sockaddr_in addr_ = Posix::toSockAddr(addr);

    if (::bind(fd, (struct sockaddr *)&addr_, sizeof(addr_)) < 0)
        return Posix::fromLastErrno();

    if (::listen(fd, 128) < 0)
        return Posix::fromLastErrno();

    return Ok(makeStrong<Posix::Fd>(fd));
}

Res<Strong<Sys::Fd>> listenIpc(Mime::Url url) {
    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return Posix::fromLastErrno();

    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    String path = try$(resolve(url)).str();
    logDebug("Listening on {}", path);
    auto sunPath = MutSlice(addr.sun_path, sizeof(addr.sun_path) - 1);
    copy(sub(path), sunPath);

    if (::bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        return Posix::fromLastErrno();

    if (::listen(fd, 128) < 0)
        return Posix::fromLastErrno();

    return Ok(makeStrong<Posix::Fd>(fd));
}

// MARK: Time ------------------------------------------------------------------

TimeSpan fromTimeSpec(struct timespec const &ts) {
    auto usecs = (u64)ts.tv_sec * 1000000 + (u64)ts.tv_nsec / 1000;
    return TimeSpan::fromUSecs(usecs);
}

TimeStamp now() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return TimeStamp::epoch() + fromTimeSpec(ts);
}

TimeSpan uptime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return fromTimeSpec(ts);
}

// MARK: Memory Managment ------------------------------------------------------

isize mmapOptionsToProt(Sys::MmapOptions const &options) {
    isize prot = 0;

    if (options.flags & Sys::MmapFlags::READ)
        prot |= PROT_READ;

    if (options.flags & Sys::MmapFlags::WRITE)
        prot |= PROT_WRITE;

    if (options.flags & Sys::MmapFlags::EXEC)
        prot |= PROT_EXEC;

    return prot;
}

Res<Sys::MmapResult> memMap(Karm::Sys::MmapOptions const &options) {
    void *addr = mmap((void *)options.vaddr, options.size, mmapOptionsToProt(options), MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (addr == MAP_FAILED)
        return Posix::fromLastErrno();

    if (options.flags & Sys::MmapFlags::PREFETCH) {
        if (madvise(addr, options.size, MADV_WILLNEED) < 0)
            return Posix::fromLastErrno();
    }

    return Ok(Sys::MmapResult{0, (usize)addr, (usize)options.size});
}

Res<Sys::MmapResult> memMap(Sys::MmapOptions const &options, Strong<Sys::Fd> maybeFd) {
    Strong<Posix::Fd> fd = try$(maybeFd.cast<Posix::Fd>());
    usize size = options.size;

    if (size == 0)
        size = try$(Io::size(*fd));

    void *addr = mmap((void *)options.vaddr, size, mmapOptionsToProt(options), MAP_SHARED, fd->_raw, options.offset);

    if (addr == MAP_FAILED)
        return Posix::fromLastErrno();

    return Ok(Sys::MmapResult{0, (usize)addr, (usize)size});
}

Res<> memUnmap(void const *buf, usize len) {
    if (::munmap((void *)buf, len) < 0)
        return Posix::fromLastErrno();

    return Ok();
}

Res<> memFlush(void *flush, usize len) {
    if (::msync(flush, len, MS_INVALIDATE) < 0)
        return Posix::fromLastErrno();

    return Ok();
}

Res<> populate(Sys::SysInfo &infos) {
    struct utsname uts;
    if (uname(&uts) < 0)
        return Posix::fromLastErrno();

    infos.sysName = "Posix"s;
    infos.sysVersion = try$(Io::format("{}", _POSIX_VERSION));

    infos.kernelName = Str::fromNullterminated(uts.sysname, sizeof(uts.sysname));
    infos.kernelVersion = Str::fromNullterminated(uts.release, sizeof(uts.release));

    infos.hostname = Str::fromNullterminated(uts.nodename, sizeof(uts.nodename));

    return Ok();
}

Res<> populate(Sys::MemInfo &) {
    return Error::notImplemented();
}

Res<> populate(Vec<Sys::CpuInfo> &) {
    return Error::notImplemented();
}

Res<> populate(Sys::UserInfo &infos) {
    infos.name = Str::fromNullterminated(getenv("USER"));
    infos.home.scheme = "file"s;
    infos.home.path = Mime::Path::parse(getenv("HOME"));

    infos.shell.scheme = "file"s;
    infos.shell.path = Mime::Path::parse(getenv("SHELL"));

    return Ok();
}

Res<> populate(Vec<Sys::UserInfo> &infos) {
    Sys::UserInfo info;
    try$(populate(info));
    infos.pushBack(info);
    return Ok();
}

// MARK: Process Managment -----------------------------------------------------

Res<> sleep(TimeSpan span) {
    struct timespec ts;
    ts.tv_sec = span.toSecs();
    ts.tv_nsec = (span.toUSecs() % 1000000) * 1000;
    if (nanosleep(&ts, nullptr) < 0)
        return Posix::fromLastErrno();
    return Ok();
}

Res<> exit(i32 res) {
    ::exit(res);
    return Ok();
}

} // namespace Karm::Sys::_Embed
