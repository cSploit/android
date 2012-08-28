package it.evilsocket.dsploit.system;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.net.Target;

public abstract class Plugin extends Activity 
{
	private String        mName				  = null;
	private String		  mDescription		  = null;
	private Target.Type[] mAllowedTargetTypes = null;
	private int			  mLayoutId			  = 0;
	private int			  mIconId		      = 0;
		
	public Plugin( String name, String description, Target.Type[] allowedTargetTypes, int layoutId, int iconResourceId ){
		mName 				= name;
		mDescription	    = description;
		mAllowedTargetTypes = allowedTargetTypes;
		mLayoutId			= layoutId;
		mIconId				= iconResourceId;
	}
	
	public Plugin( String name, String description, Target.Type[] allowedTargetTypes, int layoutId ){
		this( name, description, allowedTargetTypes, layoutId, R.drawable.action_plugin_48 );
	}
	
	public String getName(){
		return mName;
	}
	
	public String getDescription(){
		return mDescription;
	}
	
	public Target.Type[] getAllowedTargetTypes(){
		return mAllowedTargetTypes;
	}
	
	public int getIconResourceId(){
		return mIconId;
	}
	
	public boolean isAllowedTarget( Target target ){
		for( Target.Type type : mAllowedTargetTypes )
			if( type == target.getType() )
				return true;
		
		return false;
	}	

	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);     
        setTitle( mName );
        setContentView( mLayoutId );        
	}
	
	@Override
	public void onBackPressed() {
	    super.onBackPressed();
	    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
	}
	
	public void onTargetNewOpenPort( Target target, Target.Port port ) {
		
	}
}
