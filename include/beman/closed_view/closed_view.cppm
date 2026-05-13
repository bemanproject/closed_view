module;

#include <version>

export module beman.closed_view;

import std;

#define BEMAN_CLOSED_VIEW_INCLUDED_FROM_INTERFACE_UNIT
export {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winclude-angled-in-module-purview"
#include <beman/closed_view/closed.hpp>
#pragma clang diagnostic pop
}
