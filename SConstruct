#!/usr/bin/env python
import os
import sys

from methods import print_error


libname = "godot_win_toaster"
projectdir = "project"

# Godot version the bindings are generated against. godot-cpp supports 4.3 to 4.7.
# Keep this at the lowest engine version you want to support: a library built
# against 4.3 still loads in 4.4+, as long as compatibility_minimum in
# project/bin/godot_win_toaster.gdextension matches.
api_version = "4.3"

localEnv = Environment(tools=["default"], PLATFORM="")

# Build profiles can be used to decrease compile times.
# You can either specify "disabled_classes", OR
# explicitly specify "enabled_classes" which disables all other classes.
# Modify the example file as needed and uncomment the line below or
# manually specify the build_profile parameter when running SCons.

# localEnv["build_profile"] = "build_profile.json"

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()

if not (os.path.isdir("godot-cpp") and os.listdir("godot-cpp")):
    print_error("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

env = SConscript("godot-cpp/SConstruct", {"api_version": api_version, "env": env, "customs": customs})

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

# WinToast, vendored under thirdparty/WinToast.
wintoast_dir = "thirdparty/WinToast"
env.Append(CPPPATH=[wintoast_dir + "/include"])
sources += Glob(wintoast_dir + "/src/*.cpp")

if env["platform"] == "windows":
	env.Append(CPPDEFINES=["UNICODE", "_UNICODE"])
	# shlwapi/user32 are also referenced by #pragma comment in wintoastlib.cpp;
	# listing them here keeps MinGW builds working as well.
	env.Append(LIBS=["psapi", "shlwapi", "user32"])

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

# .dev doesn't inhibit compatibility, so we don't need to key it.
# .universal just means "compatible with all relevant arches" so we don't need to key it.
suffix = env['suffix'].replace(".dev", "").replace(".universal", "")

lib_filename = "{}{}{}{}".format(env.subst('$SHLIBPREFIX'), libname, suffix, env.subst('$SHLIBSUFFIX'))

library = env.SharedLibrary(
    "bin/{}/{}".format(env['platform'], lib_filename),
    source=sources,
)

copy = env.Install("{}/bin/{}/".format(projectdir, env["platform"]), library)

default_args = [library, copy]
Default(*default_args)
