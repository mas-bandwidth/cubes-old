/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#include "PreCompiled.h"
#include "network/netSockets.h"
#include "demos/SingleplayerDemo.h"
/*
#include "demos/MultiplayerDemo.h"
#include "demos/InterpolationDemo.h"
#include "demos/StateReplicationDemo.h"
#include "demos/AuthorityDemo.h"
#include "demos/CorrectionsDemo.h"
*/

Demo * CreateDemo( int index )
{
	if ( index == 0 )
		return new SingleplayerDemo();

/*		
	if ( index == 1 )
		return new MultiplayerDemo();
		
	if ( index == 2 )
		return new InterpolationDemo();
		
	if ( index == 3 )
		return new StateReplicationDemo();
		
	if ( index == 4 )
		return new AuthorityDemo();
		
	if ( index == 5 )
		return new CorrectionsDemo();
*/

	return NULL;
}

//#define PROFILE

int main( int argc, char * argv[] )
{	
	printf( "networked physics demo\n" );

	net::InitializeSockets();
	while ( !net::IsInitialized() )
	{
		printf( "error: failed to initialize sockets\n" );
		net::ShutdownSockets();
		return 1;
	}

#ifndef PROFILE
	
	int displayWidth, displayHeight;
	GetDisplayResolution( displayWidth, displayHeight );
	printf( "display resolution is %d x %d\n", displayWidth, displayHeight );

	HideMouseCursor();
	
	if ( !OpenDisplay( "Networked Physics", displayWidth, displayHeight ) )
	{
		printf( "error: failed to open display" );
		return 1;
	}
	
#endif

	int currentDemo = 0;
	Demo * demo = CreateDemo( 0 );
	assert( demo );
	demo->InitializeWorld();	
    #ifndef PROFILE
	demo->InitializeRender( displayWidth, displayHeight );
    #endif

	bool shadows = true;

	uint32_t frame = 0;
	
	while ( true )
	{
		#ifdef PROFILE
		printf( "profiling frame %d\n", frame );
		#endif
		
		platform::Input input = platform::Input::Sample();

		#ifdef PROFILE
		if ( frame > 500 )
			input.left = frame % 2;
		else if ( frame > 100 && ( frame % 5 ) == 0 )
			input.left  = true;
		input.z = true;
		#endif
		
		if ( input.alt )
		{
			int demoIndex = -1;
			
			if ( input.one )
				demoIndex = 0;
				
			if ( input.two )
				demoIndex = 1;
				
			if ( input.three )
				demoIndex = 2;
				
			if ( input.four )
				demoIndex = 3;
				
			if ( input.five )
				demoIndex = 4;
				
			if ( input.six )
				demoIndex = 5;
				
			if ( input.seven )
				demoIndex = 6;
				
			if ( input.eight )
				demoIndex = 7;
				
			if ( input.nine )
				demoIndex = 8;
				
			if ( input.zero )
				demoIndex = 9;

			static bool enterDownLastFrame = false;
			if ( input.enter && !enterDownLastFrame )
				shadows = !shadows;
			enterDownLastFrame = input.enter;
				
			if ( demoIndex != -1 )
			{
				Demo * newDemo = CreateDemo( demoIndex );
				if ( newDemo )
				{
					#ifndef PROFILE
					ClearDisplay( displayWidth, displayHeight );
					#endif
					delete demo;
					demo = newDemo;
					assert( demo );
					demo->InitializeWorld();
					#ifndef PROFILE
					demo->InitializeRender( displayWidth, displayHeight );
					#endif
					currentDemo = demoIndex;
				}
			}
		}
		
		static bool escapeDownLastFrame = false;		
		if ( input.escape && !escapeDownLastFrame )
		{
			#ifndef PROFILE
			ClearDisplay( displayWidth, displayHeight );
			#endif
			delete demo;
			demo = CreateDemo( currentDemo );
			assert( demo );
			demo->InitializeWorld();
			#ifndef PROFILE
			demo->InitializeRender( displayWidth, displayHeight );
			#endif
		}
		escapeDownLastFrame = input.escape;
		
		if ( frame > 50 )
		{
			demo->ProcessInput( !input.alt ? input : platform::Input() );
			demo->Update( DeltaTime );
            demo->WaitForSim();
		}
		
		#ifndef PROFILE
		demo->Render( DeltaTime, shadows );
		UpdateDisplay( 1 );
		#endif
				
		frame ++;
	}

    #ifndef PROFILE
	CloseDisplay();
    #endif

	delete demo;

	printf( "shutdown\n" );
	
	net::ShutdownSockets();

	return 0;
}
