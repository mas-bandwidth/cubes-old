/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#include "demos/Demo.h"
#include "shared/Hypercube.h"

class DeterministicLockstepDemo : public Demo
{
private:

	render::Render * render;

    struct Instance
    {
        game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * game;

        float t;            // NETHACK: using floating point time is a very bad idea
        Camera camera;
        math::Vector origin;
        GameWorkerThread workerThread;

        view::Packet viewPacket;
        view::ObjectManager viewObjectManager;
    };

    static const int NumInstances = 2;

    Instance instance[NumInstances];
	
public:

	enum { steps = 1024 };

	DeterministicLockstepDemo()
	{
		game::Config config;
		config.maxObjects = steps * steps + MaxPlayers + 1;
		config.deactivationTime = 0.5f;
		config.cellSize = 4.0f;
		config.cellWidth = steps / config.cellSize + 2;
		config.cellHeight = config.cellWidth;
		config.activationDistance = 5.0f;
		config.simConfig.ERP = 0.1f;
		config.simConfig.CFM = 0.001f;
		config.simConfig.MaxIterations = 12;
		config.simConfig.MaximumCorrectingVelocity = 100.0f;
		config.simConfig.ContactSurfaceLayer = 0.05f;
		config.simConfig.Elasticity = 0.3f;
		config.simConfig.LinearDrag = 0.01f;
		config.simConfig.AngularDrag = 0.01f;
		config.simConfig.Friction = 200.0f;

        for ( int i = 0; i < NumInstances; ++i )
        {
    		instance[i].game = new game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> ( config );
    		instance[i].t = 0.0f;
    		instance[i].origin = math::Vector(0,0,0);
        }

        render = NULL;
	}
	
	~DeterministicLockstepDemo()
	{
        for ( int i = 0; i < NumInstances; ++i )
		  delete instance[i].game;
		delete render;
	}
	
	void InitializeWorld()
	{
		for ( int i = 0; i < NumInstances; ++i )
        {
            instance[i].game->InitializeBegin();

    		instance[i].game->AddPlane( math::Vector(0,0,1), 0 );

    		AddCube( instance[i].game, 1, math::Vector(0,0,10) );

    		const int border = 10.0f;
    		const float origin = -steps / 2 + border;
    		const float z = hypercube::NonPlayerCubeSize / 2;
    		const int count = steps - border * 2;
    		for ( int y = 0; y < count; ++y )
    			for ( int x = 0; x < count; ++x )
    				AddCube( instance[i].game, 0, math::Vector(x+origin,y+origin,z) );

    		instance[i].game->InitializeEnd();

    		instance[i].game->OnPlayerJoined( 0 );
    		instance[i].game->SetLocalPlayer( 0 );
    		instance[i].game->SetPlayerFocus( 0, 1 );
    	
    		instance[i].game->SetFlag( game::FLAG_Push );
    		instance[i].game->SetFlag( game::FLAG_Pull );
        }
	}
	
	void InitializeRender( int displayWidth, int displayHeight )
	{
		render = new render::Render( displayWidth, displayHeight );
	}
    
	void ResizeDisplay( int displayWidth, int displayHeight )
	{
		render->ResizeDisplay( displayWidth, displayHeight );
	}

	void AddCube( game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * gameInstance, int player, const math::Vector & position )
	{
		hypercube::DatabaseObject object;
		CompressPosition( position, object.position );
		CompressOrientation( math::Quaternion(1,0,0,0), object.orientation );
		object.dirty = player;
		object.enabled = player;
		object.session = 0;
		object.player = player;
		ObjectId id = gameInstance->AddObject( object, position.x, position.y );
		if ( player )
			gameInstance->DisableObject( id );
	}

	void ProcessInput( const platform::Input & input )
	{
		// pass input to game instance
		
		game::Input gameInput;
		gameInput.left = input.left ? 1.0f : 0.0f;
		gameInput.right = input.right ? 1.0f : 0.0f;
		gameInput.up = input.up ? 1.0f : 0.0f;
		gameInput.down = input.down ? 1.0f : 0.0f;
		gameInput.push = input.space ? 1.0f : 0.0f;
		gameInput.pull = input.z ? 1.0f : 0.0f;

        // NETHACK: todo - actually we want to stagger the input for the second instance
        for ( int i = 0; i < NumInstances; ++i )
            instance[i].game->SetPlayerInput( 0, gameInput );
	}
	
	void Update( float deltaTime )
	{
        for ( int i = 0; i < NumInstances; ++i )
        {
    		// update camera
    		view::Object * playerCube = instance[i].viewObjectManager.GetObject( 1 );
    		if ( playerCube )
    			instance[i].origin = playerCube->position + playerCube->positionError;
    		math::Vector lookat = instance[i].origin - math::Vector(0,0,1);
    		#ifdef WIDESCREEN
    		math::Vector position = lookat + math::Vector(0,-11,5);
    		#else
    		math::Vector position = lookat + math::Vector(0,-12,6);
    		#endif
    		instance[i].camera.EaseIn( lookat, position ); 

    		// start the worker thread
    		instance[i].workerThread.Start( instance[i].game );
    		instance[i].t += deltaTime;
        }
	}

    void WaitForSim()
    {
        for ( int i = 0; i < NumInstances; ++i )
        {
            instance[i].workerThread.Join();
            instance[i].game->GetViewPacket( instance[i].viewPacket );
        }
    }
    	
	void Render( float deltaTime, bool shadows )
	{
		// update the scene to be rendered
		
        for ( int i = 0; i < NumInstances; ++i )
        {
    		if ( instance[i].viewPacket.objectCount >= 1 )
    		{
    			view::ObjectUpdate updates[MaxViewObjects];
    			getViewObjectUpdates( updates, instance[i].viewPacket );
    			instance[i].viewObjectManager.UpdateObjects( updates, instance[i].viewPacket.objectCount );
    		}

    		instance[i].viewObjectManager.ExtrapolateObjects( deltaTime );

    		instance[i].viewObjectManager.Update( deltaTime );
        }

        // prepare for rendering

        render->ClearScreen();

        int width = render->GetDisplayWidth();
        int height = render->GetDisplayHeight();

        // render the local view (left)

        Cubes cubes;
        instance[0].viewObjectManager.GetRenderState( cubes );
        setCameraAndLight( render, instance[0].camera );
        render->BeginScene( 0, 0, width/2, height );
        ActivationArea activationArea;
        setupActivationArea( activationArea, instance[0].origin, 5.0f, instance[0].t );
        render->RenderActivationArea( activationArea, 1.0f );
        render->RenderCubes( cubes );
        #ifdef SHADOWS
        if ( shadows )
            render->RenderCubeShadows( cubes );
        #endif

        // render the remote view (right)

        instance[1].viewObjectManager.GetRenderState( cubes );
        setCameraAndLight( render, instance[1].camera );
        render->BeginScene( width/2, 0, width, height );
        setupActivationArea( activationArea, instance[1].origin, 5.0f, instance[0].t );
        render->RenderActivationArea( activationArea, 1.0f );
        render->RenderCubes( cubes );
        #ifdef SHADOWS
        if ( shadows )
            render->RenderCubeShadows( cubes );
        #endif

        // shadow quad on top

        #ifdef SHADOWS
        if ( shadows )
            render->RenderShadowQuad();
        #endif

        // divide splitscreen

        render->DivideSplitscreen();

        // letterbox last of all

        #ifdef LETTERBOX
        render->LetterBox( 80 );
        #endif
	}
};
