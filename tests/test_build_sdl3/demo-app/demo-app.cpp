#include <SDL3/SDL.h>

int main( int argc, char** argv ) {
	SDL_Window* window = SDL_CreateWindow( "Built SDL3 from source demo", 1280, 720, 0 );

	Uint64 lastTime = SDL_GetTicks();

	while ( 1 ) {
		Uint64 now = SDL_GetTicks();

		if ( now - lastTime > 3000 ) {
			break;
		}
	}

	SDL_DestroyWindow( window );
	window = NULL;

	return 0;
}