#pragma once

#include <web-dom/comment.h>
#include <web-dom/document-type.h>
#include <web-dom/document.h>
#include <web-dom/element.h>
#include <web-dom/text.h>

namespace Web::Xml {

struct Parser {
    Res<Strong<Dom::Document>> parse(Io::SScan &s, Ns ns);

    Res<> _parseS(Io::SScan &s);

    Res<Str> _parseName(Io::SScan &s);

    Res<> _parseCharData(Io::SScan &s, StringBuilder &sb);

    Res<Strong<Dom::Comment>> _parseComment(Io::SScan &s);

    Res<> _parsePi(Io::SScan &s);

    Res<> _parsePiTarget(Io::SScan &s);

    Res<> _parseCDSect(Io::SScan &s, StringBuilder &sb);

    Res<> _parseVersionInfo(Io::SScan &s);

    Res<> _parseXmlDecl(Io::SScan &s);

    Res<> _parseMisc(Io::SScan &s, Dom::Node &parent);

    Res<> _parseProlog(Io::SScan &s, Dom::Node &parent);

    Res<Strong<Dom::DocumentType>> _parseDoctype(Io::SScan &s);

    Res<Strong<Dom::Element>> _parseElement(Io::SScan &s, Ns ns);

    Res<Strong<Dom::Element>> _parseStartTag(Io::SScan &s, Ns ns);

    Res<> _parseAttribute(Io::SScan &s, Ns ns, Dom::Element &el);

    Res<String> _parseAttValue(Io::SScan &s);

    Res<> _parseEndTag(Io::SScan &s, Dom::Element &el);

    Res<> _parseContentItem(Io::SScan &s, Ns ns, Dom::Element &el);

    Res<> _parseContent(Io::SScan &s, Ns ns, Dom::Element &el);

    Res<> _parseTextItem(Io::SScan &s, StringBuilder &sb);

    Res<> _parseText(Io::SScan &s, Dom::Element &el);

    Res<Strong<Dom::Element>> _parseEmptyElementTag(Io::SScan &s, Ns ns);

    Res<Rune> _parseCharRef(Io::SScan &s);

    Res<Rune> _parseEntityRef(Io::SScan &s);

    Res<Rune> _parseReference(Io::SScan &s);

    Res<> _parseExternalId(Io::SScan &s, Dom::DocumentType &docType);
};

} // namespace Web::Xml
