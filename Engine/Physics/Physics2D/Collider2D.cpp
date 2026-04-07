#include "Collider2D.hpp"

// All collider method implementations are inline in the header or trivial.
// This translation unit exists so that the linker has a home for vtables
// when the collider classes are used via shared_ptr / polymorphism.

namespace GameEngine {
namespace Physics2D {

    // Force vtable emission in this TU for the base class.
    // (destructor is virtual and defaulted in the header, but having at least
    //  one non-inline virtual method is best practice.  We keep the header lean
    //  and define nothing extra here; the implicit destructor suffices for now.)

}}
