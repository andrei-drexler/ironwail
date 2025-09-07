# Open Watcom makefile to generate SDL2.lib
# def2lbc.awk (from OW source tree) requires 'awk'
# run 'wmake'

all: SDL2.lib

SDL2.exp: SDL2.def def2lbc.awk
	awk -v cpu=386 -f def2lbc.awk SDL2.def > SDL2.exp

SDL2.lib: SDL2.exp
	wlib -q -b -n -c -pa -s -t -zld -ii -io SDL2.lib @SDL2.exp
