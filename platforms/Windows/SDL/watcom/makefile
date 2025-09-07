# Open Watcom makefile to generate SDL.lib
# def2lbc.awk (from OW source tree) requires 'awk'
# run 'wmake'

all: SDL.lib

SDL.exp: SDL.def def2lbc.awk
	awk -v cpu=386 -f def2lbc.awk SDL.def > SDL.exp

SDL.lib: SDL.exp
	wlib -q -b -n -c -pa -s -t -zld -ii -io SDL.lib @SDL.exp
