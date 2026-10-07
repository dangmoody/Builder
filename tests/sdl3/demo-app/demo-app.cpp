#include <SDL3/SDL.h>

#include <stdio.h>

enum {
	WINDOW_WIDTH		= 1280,
	WINDOW_HEIGHT		= 720,
	RUN_DURATION_MS		= 3000,
};

int main( int argc, char **argv ) {
	(void) argc;
	(void) argv;

	if ( !SDL_Init( SDL_INIT_VIDEO ) ) {
		printf( "SDL_Init failed: %s\n", SDL_GetError() );
		return 1;
	}

	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;

	if ( !SDL_CreateWindowAndRenderer( "Built SDL3 from source demo", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer ) ) {
		printf( "SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError() );
		SDL_Quit();
		return 1;
	}

	printf( "Video driver: %s, renderer: %s\n", SDL_GetCurrentVideoDriver(), SDL_GetRendererName( renderer ) );

	Uint64 startTime = SDL_GetTicks();
	bool running = true;

	while ( running ) {
		// pump events so the window actually gets mapped and stays responsive
		// SDL_Event event;
		// while ( SDL_PollEvent( &event ) ) {
		// 	if ( event.type == SDL_EVENT_QUIT ) {
		// 		running = false;
		// 	}
		// }

		if ( SDL_GetTicks() - startTime > RUN_DURATION_MS ) {
			running = false;
		}

		// wayland wont show the window until a buffer is presented
		SDL_SetRenderDrawColor( renderer, 100, 149, 237, 255 );
		SDL_RenderClear( renderer );
		SDL_RenderPresent( renderer );
	}

	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
	SDL_Quit();

	return 0;
}
