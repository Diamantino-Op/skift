from pathlib import Path
from . import image, store, runner
from cutekit import cli, model, shell, const


def generateSystem(img: image.Image) -> None:
    img.mkdir("objects")
    img.mkdir("bundles")

    img.mkdir("EFI")
    img.mkdir("EFI/BOOT")
    img.installTo("loader", "efi-x86_64", "EFI/BOOT/BOOTX64.EFI")

    img.install("hjert", "kernel-x86_64")

    img.install("grund-system", "skift-x86_64")
    img.install("grund-device", "skift-x86_64")
    img.install("hideo-shell", "skift-x86_64")
    img.install("skift-branding", "skift-x86_64")
    img.install("skift-wallpapers", "skift-x86_64")
    img.install("inter-font", "skift-x86_64")
    img.install("mdi-font", "skift-x86_64")

    img.cpTree("meta/image/boot", "boot")


@cli.command("i", "image", "Generate the boot image")
def _() -> None:
    pass


class BuildArgs:
    debug: bool = cli.arg(None, "debug", "Build the image in debug mode")
    fmt: str = cli.arg(None, "format", "The format of the image")
    compress: str = cli.arg(None, "compress", "Compress the image")
    dist: bool = cli.arg(None, "dist", "Copy the image to the dist folder")


@cli.command("g", "image/build", "Generate the boot image")
def _(args: BuildArgs) -> None:

    rargs = cli.defaults(model.RegistryArgs)
    rargs.mixins.append("release" if not args.debug else "debug")

    registry = model.Registry.use(rargs)

    s = (
        store.Dir("image-efi-x86_64")
        if args.fmt == "dir"
        else store.RawHdd("image-efi-x86_64", 256)
    )
    img = image.Image(registry, s)
    generateSystem(img)

    file = img.finalize()
    if args.fmt == "hdd" and args.compress:
        zip = shell.compress(file, format=args.compress)
        shell.rmrf(file)
        file = zip

    if args.dist:
        distDir = Path(const.PROJECT_CK_DIR) / "dist"
        shell.mkdir(distDir)
        distPath = distDir / Path(file).name
        shell.cp(file, distPath)
        file = distPath

    print(file)


class BuildArgs:
    debug: bool = cli.arg(None, "debug", "Build the image in debug mode")
    arch: str = cli.arg(
        None,
        "arch",
        "The architecture of the image (x86_64 or riscv32)",
        default="x86_64",
    )
    dint: bool = cli.arg(None, "dint", "Debug interrupts")


@cli.command("s", "image/start", "Boot the system")
def _(args: BuildArgs) -> None:
    if args.arch not in ["x86_64", "riscv32"]:
        raise RuntimeError("Unknown arch")
    rargs = cli.defaults(model.RegistryArgs)
    rargs.mixins.append("release" if not args.debug else "debug")

    registry = model.Registry.use(rargs)

    img = image.Image(registry, store.Dir(f"image-efi-{args.arch}"))

    if args.arch == "x86_64":
        generateSystem(img)
        machine = runner.Qemu(logError=args.dint, debugger=args.debug)
        machine.boot(img)
    elif args.arch == "riscv32":
        kernel = img.install("hjert-riscv", "kernel-riscv32")
        qemu = [
            "qemu-system-riscv32",
            "-machine",
            "virt",
            "-bios",
            "default",
            "-nographic",
            "-serial",
            "mon:stdio",
            "--no-reboot",
            "-kernel",
            kernel.path,
        ]
        shell.exec(*qemu)
