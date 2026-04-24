#!/bin/bash
clear

Folder="../builddir"
Program="reshadeVK"

if [ ! -d "$Folder" ]; then
	echo "Ambiente faltando.."
	./build.sh
fi

cd ..

echo "Compilando projeto.."
meson compile -C builddir
echo "Compilação concluida"

echo "iniciando $Program (Verifique seu gerenciador de tarefas..)"
./builddir/$Program
echo "$Program encerrado"