add_rules("mode.debug", "mode.release")

target("example")
    set_kind("binary")
    add_files("src/*.cpp")
    add_includedirs("../single")

    after_build(function (target)
        os.cp(target:targetfile(), "$(projectdir)/")
    end)