/* Copyright (c) Facebook, Inc. and its affiliates. */
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

// "digit" is declared in both Python's longintrepr.h and NetHack's extern.h.
#define digit nethack_digit

extern "C" {
#include "hack.h"
#include "permonst.h"
#include "pm.h" // File generated during NetHack compilation.
#include "rm.h"
}

extern "C" {
#include "nledl.h"
}

// Undef name clashes between NetHack and Python.
#undef yn
#undef min
#undef max

// From drawing.c. Needs drawing.o at link time.
// extern const struct class_sym def_monsyms[MAXMCLASSES];

namespace py = pybind11;

template <typename T>
T *
checked_conversion(py::object obj, const std::vector<ssize_t> &shape)
{
    if (obj.is_none())
        return nullptr;
    auto array = py::array::ensure(obj.release());
    if (!array)
        throw std::runtime_error("Numpy array required");

    // We don't use py::array_t<T> (or <T, 0>) above as that still
    // causes conversions to "larger" types.

    // TODO: Better error messages here and below.
    if (!array.dtype().is(py::dtype::of<T>()))
        throw std::runtime_error("Numpy array of right type required");

    py::buffer_info buf = array.request();

    if (buf.ndim != shape.size())
        throw std::runtime_error("array has wrong number of dims");
    if (!std::equal(shape.begin(), shape.end(), buf.shape.begin()))
        throw std::runtime_error("Array has wrong shape");
    if (!(array.flags() & py::array::c_style))
        throw std::runtime_error("Array isn't C contiguous");

    return static_cast<T *>(buf.ptr);
}

class Nethack
{
  public:
    Nethack(std::string dlpath)
        : dlpath_(std::move(dlpath)), obs_{ 0,       0,       nullptr,
                                            nullptr, nullptr, nullptr,
                                            nullptr }
    {
    }
    ~Nethack()
    {
        close();
    }
    void
    step(int action)
    {
        if (obs_.done) {
            throw std::runtime_error("Called step on finished NetHack");
        }
        obs_.action = action;
        nle_ = nle_step(nle_, &obs_);
    }
    bool
    done()
    {
        return obs_.done;
    }

    void
    reset()
    {
        if (!nle_) {
            nle_ = nle_start(dlpath_.c_str(), &obs_);
            return;
        }
        nle_reset(nle_, &obs_);
    }

    void
    set_buffers(py::object glyphs, py::object chars, py::object colors,
                py::object specials, py::object blstats,
                py::object program_state)
    {
        std::vector<ssize_t> dungeon{ ROWNO, COLNO - 1 };
        obs_.glyphs = checked_conversion<int16_t>(std::move(glyphs), dungeon);
        obs_.chars = checked_conversion<uint8_t>(std::move(chars), dungeon);
        obs_.colors = checked_conversion<uint8_t>(std::move(colors), dungeon);
        obs_.specials =
            checked_conversion<uint8_t>(std::move(specials), dungeon);
        obs_.blstats = checked_conversion<long>(std::move(blstats), { 23 });
        obs_.program_state =
            checked_conversion<int>(std::move(program_state), { 5 });
    }

    void
    close()
    {
        if (nle_) {
            nle_end(nle_);
            nle_ = nullptr;
        }
    }

  private:
    std::string dlpath_;
    nle_obs obs_;
    nle_ctx_t *nle_ = nullptr;
};

PYBIND11_MODULE(_pynethack, m)
{
    m.doc() = "The NetHack Learning Environment";

    py::class_<Nethack>(m, "Nethack")
        .def(py::init<const char *>())
        .def("step", &Nethack::step, py::arg("action"))
        .def("done", &Nethack::done)
        .def("reset", &Nethack::reset)
        .def("set_buffers", &Nethack::set_buffers,
             py::arg("glyphs") = py::none(), py::arg("chars") = py::none(),
             py::arg("colors") = py::none(), py::arg("specials") = py::none(),
             py::arg("blstats") = py::none(),
             py::arg("program_state") = py::none(), py::keep_alive<1, 2>(),
             py::keep_alive<1, 3>(), py::keep_alive<1, 4>(),
             py::keep_alive<1, 5>(), py::keep_alive<1, 6>(),
             py::keep_alive<1, 7>())
        .def("close", &Nethack::close);

    py::module mn = m.def_submodule(
        "nethack", "Collection of NetHack constants and functions");

    mn.attr("NHW_MESSAGE") = py::int_(NHW_MESSAGE);
    mn.attr("NHW_STATUS") = py::int_(NHW_STATUS);
    mn.attr("NHW_MAP") = py::int_(NHW_MAP);
    mn.attr("NHW_MENU") = py::int_(NHW_MENU);
    mn.attr("NHW_TEXT") = py::int_(NHW_TEXT);

    // mn.attr("MAXWIN") = py::int_(MAXWIN);

    mn.attr("NUMMONS") = py::int_(NUMMONS);

    // Glyph array offsets. This is what the glyph_is_* functions
    // are based on, see display.h.
    mn.attr("GLYPH_MON_OFF") = py::int_(GLYPH_MON_OFF);
    mn.attr("GLYPH_PET_OFF") = py::int_(GLYPH_PET_OFF);
    mn.attr("GLYPH_INVIS_OFF") = py::int_(GLYPH_INVIS_OFF);
    mn.attr("GLYPH_DETECT_OFF") = py::int_(GLYPH_DETECT_OFF);
    mn.attr("GLYPH_BODY_OFF") = py::int_(GLYPH_BODY_OFF);
    mn.attr("GLYPH_RIDDEN_OFF") = py::int_(GLYPH_RIDDEN_OFF);
    mn.attr("GLYPH_OBJ_OFF") = py::int_(GLYPH_OBJ_OFF);
    mn.attr("GLYPH_CMAP_OFF") = py::int_(GLYPH_CMAP_OFF);
    mn.attr("GLYPH_EXPLODE_OFF") = py::int_(GLYPH_EXPLODE_OFF);
    mn.attr("GLYPH_ZAP_OFF") = py::int_(GLYPH_ZAP_OFF);
    mn.attr("GLYPH_SWALLOW_OFF") = py::int_(GLYPH_SWALLOW_OFF);
    mn.attr("GLYPH_WARNING_OFF") = py::int_(GLYPH_WARNING_OFF);
    mn.attr("GLYPH_STATUE_OFF") = py::int_(GLYPH_STATUE_OFF);
    mn.attr("MAX_GLYPH") = py::int_(MAX_GLYPH);

    mn.attr("NO_GLYPH") = py::int_(NO_GLYPH);
    mn.attr("GLYPH_INVISIBLE") = py::int_(GLYPH_INVISIBLE);

    mn.attr("MAXPCHARS") = py::int_(static_cast<int>(MAXPCHARS));
    mn.attr("EXPL_MAX") = py::int_(static_cast<int>(EXPL_MAX));
    mn.attr("NUM_ZAP") = py::int_(static_cast<int>(NUM_ZAP));
    mn.attr("WARNCOUNT") = py::int_(static_cast<int>(WARNCOUNT));

    // From objclass.h
    mn.attr("RANDOM_CLASS") = py::int_(static_cast<int>(
        RANDOM_CLASS)); /* used for generating random objects */
    mn.attr("ILLOBJ_CLASS") = py::int_(static_cast<int>(ILLOBJ_CLASS));
    mn.attr("WEAPON_CLASS") = py::int_(static_cast<int>(WEAPON_CLASS));
    mn.attr("ARMOR_CLASS") = py::int_(static_cast<int>(ARMOR_CLASS));
    mn.attr("RING_CLASS") = py::int_(static_cast<int>(RING_CLASS));
    mn.attr("AMULET_CLASS") = py::int_(static_cast<int>(AMULET_CLASS));
    mn.attr("TOOL_CLASS") = py::int_(static_cast<int>(TOOL_CLASS));
    mn.attr("FOOD_CLASS") = py::int_(static_cast<int>(FOOD_CLASS));
    mn.attr("POTION_CLASS") = py::int_(static_cast<int>(POTION_CLASS));
    mn.attr("SCROLL_CLASS") = py::int_(static_cast<int>(SCROLL_CLASS));
    mn.attr("SPBOOK_CLASS") =
        py::int_(static_cast<int>(SPBOOK_CLASS)); /* actually SPELL-book */
    mn.attr("WAND_CLASS") = py::int_(static_cast<int>(WAND_CLASS));
    mn.attr("COIN_CLASS") = py::int_(static_cast<int>(COIN_CLASS));
    mn.attr("GEM_CLASS") = py::int_(static_cast<int>(GEM_CLASS));
    mn.attr("ROCK_CLASS") = py::int_(static_cast<int>(ROCK_CLASS));
    mn.attr("BALL_CLASS") = py::int_(static_cast<int>(BALL_CLASS));
    mn.attr("CHAIN_CLASS") = py::int_(static_cast<int>(CHAIN_CLASS));
    mn.attr("VENOM_CLASS") = py::int_(static_cast<int>(VENOM_CLASS));
    mn.attr("MAXOCLASSES") = py::int_(static_cast<int>(MAXOCLASSES));

    // "Special" mapglyph
    mn.attr("MG_CORPSE") = py::int_(MG_CORPSE);
    mn.attr("MG_INVIS") = py::int_(MG_INVIS);
    mn.attr("MG_DETECT") = py::int_(MG_DETECT);
    mn.attr("MG_PET") = py::int_(MG_PET);
    mn.attr("MG_RIDDEN") = py::int_(MG_RIDDEN);
    mn.attr("MG_STATUE") = py::int_(MG_STATUE);
    mn.attr("MG_OBJPILE") =
        py::int_(MG_OBJPILE); /* more than one stack of objects */
    mn.attr("MG_BW_LAVA") = py::int_(MG_BW_LAVA); /* 'black & white lava' */

    // Expose macros as Python functions.
    mn.def("glyph_is_monster",
           [](int glyph) { return glyph_is_monster(glyph); });
    mn.def("glyph_is_normal_monster",
           [](int glyph) { return glyph_is_normal_monster(glyph); });
    mn.def("glyph_is_pet", [](int glyph) { return glyph_is_pet(glyph); });
    mn.def("glyph_is_body", [](int glyph) { return glyph_is_body(glyph); });
    mn.def("glyph_is_statue",
           [](int glyph) { return glyph_is_statue(glyph); });
    mn.def("glyph_is_ridden_monster",
           [](int glyph) { return glyph_is_ridden_monster(glyph); });
    mn.def("glyph_is_detected_monster",
           [](int glyph) { return glyph_is_detected_monster(glyph); });
    mn.def("glyph_is_invisible",
           [](int glyph) { return glyph_is_invisible(glyph); });
    mn.def("glyph_is_normal_object",
           [](int glyph) { return glyph_is_normal_object(glyph); });
    mn.def("glyph_is_object",
           [](int glyph) { return glyph_is_object(glyph); });
    mn.def("glyph_is_trap", [](int glyph) { return glyph_is_trap(glyph); });
    mn.def("glyph_is_cmap", [](int glyph) { return glyph_is_cmap(glyph); });
    mn.def("glyph_is_swallow",
           [](int glyph) { return glyph_is_swallow(glyph); });
    mn.def("glyph_is_warning",
           [](int glyph) { return glyph_is_warning(glyph); });

    py::class_<permonst>(m, "permonst")
        .def_readonly("mname", &permonst::mname)   /* full name */
        .def_readonly("mlet", &permonst::mlet)     /* symbol */
        .def_readonly("mlevel", &permonst::mlevel) /* base monster level */
        .def_readonly("mmove", &permonst::mmove)   /* move speed */
        .def_readonly("ac", &permonst::ac)         /* (base) armor class */
        .def_readonly("mr", &permonst::mr) /* (base) magic resistance */
        // .def_readonly("maligntyp", &permonst::maligntyp) /* basic
        // monster alignment */
        .def_readonly("geno", &permonst::geno) /* creation/geno mask value */
        // .def_readonly("mattk", &permonst::mattk) /* attacks matrix
        // */
        .def_readonly("cwt", &permonst::cwt) /* weight of corpse */
        .def_readonly("cnutrit",
                      &permonst::cnutrit) /* its nutritional value */
        .def_readonly("msound",
                      &permonst::msound)         /* noise it makes (6 bits) */
        .def_readonly("msize", &permonst::msize) /* physical size (3 bits) */
        .def_readonly("mresists", &permonst::mresists) /* resistances */
        .def_readonly("mconveys",
                      &permonst::mconveys)           /* conveyed by eating */
        .def_readonly("mflags1", &permonst::mflags1) /* boolean bitflags */
        .def_readonly("mflags2",
                      &permonst::mflags2) /* more boolean bitflags */
        .def_readonly("mflags3",
                      &permonst::mflags3) /* yet more boolean bitflags */
#ifdef TEXTCOLOR
        .def_readonly("mcolor", &permonst::mcolor) /* color to use */
#endif
        ;

    py::class_<class_sym>(m, "class_sym")
        .def_readonly("sym", &class_sym::sym)
        .def_readonly("name", &class_sym::name)
        .def_readonly("explain", &class_sym::explain)
        .def("__repr__", [](const class_sym &cs) {
            return "<nethack.pynle.class_sym sym='" + std::string(1, cs.sym)
                   + "' explain='" + std::string(cs.explain) + "'>";
        });

    /*
    mn.def(
        "glyph_to_mon",
        [](int glyph) -> const permonst * {
            return &mons[glyph_to_mon(glyph)];
        },
        py::return_value_policy::reference);

    mn.def(
        "mlet_to_class_sym",
        [](char let) -> const class_sym * { return &def_monsyms[let]; },
        py::return_value_policy::reference);
    */
}
