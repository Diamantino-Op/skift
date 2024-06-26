#pragma once

#include <karm-ui/node.h>
#include <web-dom/document.h>

namespace Web::View {

Ui::Child view(Strong<Dom::Document> dom);

} // namespace Web::View
