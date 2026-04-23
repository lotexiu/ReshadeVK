#!/bin/bash

Folder="builddir"

if [ -d "$Folder" ]; then
	echo "Configuração de ambiente ja realizada!"
else
	echo "Configurando ambiente."
	meson setup builddir
fi
