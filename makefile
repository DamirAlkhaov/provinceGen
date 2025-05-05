make:
	gcc -Isrc/ src/main.c src/countriesMask.c src/BGR.c src/bmpLoader.c -o province.exe
	strip province.exe
	gcc -Isrc/ src/edgeDetection.c src/BGR.c src/bmpLoader.c -o edge.exe
	strip edge.exe