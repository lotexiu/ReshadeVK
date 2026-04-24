#!/bin/bash
clear
cd ..

Folder="builddir"

if [ -d "$Folder" ]; then
	echo "Limpando ambiente e arquivo anterior..."
	rm -rf $Folder
	echo "Limpeza concluida!"
else
	echo "Não há limpeza para realizar.."
fi
