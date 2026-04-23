Folder="builddir"

if [ ! -d "$Folder" ]; then
	./run_build.sh
fi

meson compile -C builddir

./builddir/reshadeVK