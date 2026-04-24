#!/bin/bash
clear
cd ..

Folder="builddir"

if [ -d "$Folder" ]; then
	echo "Configuração de ambiente ja realizada!"
else
	echo "Criando ambiente."
	meson setup builddir
	clean
	echo "Criação de ambiente concluido!"
fi
