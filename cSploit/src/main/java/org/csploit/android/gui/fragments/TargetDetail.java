package org.csploit.android.gui.fragments;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.design.widget.TabLayout;
import android.support.v4.app.Fragment;
import android.support.v4.content.ContextCompat;
import android.support.v4.view.ViewPager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import org.csploit.android.R;
import org.csploit.android.core.*;
import org.csploit.android.core.System;
import org.csploit.android.gui.adapters.TargetPagerAdapter;
import org.csploit.android.net.Target;

/**
 * A fragment that display many details about a target
 */
public class TargetDetail extends BaseFragment {

  // TODO: transform the target class into an abstract one and make
  // 3 concrete subclasses: { endpoint, network, remote }
  // or make Target an interface...
  private static final String TARGET_ID = "target_id";

  private Target target;
  private OnFragmentInteractionListener listener;

  public static TargetDetail newInstance(Target target) {
    TargetDetail fragment = new TargetDetail();
    setUp(fragment, target);
    return fragment;
  }

  public static void setUp(TargetDetail fragment, Target target) {
    Bundle args = new Bundle();
    args.putString(TARGET_ID, target.getIdentifier());
    fragment.setArguments(args);
  }

  public TargetDetail()
  { }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    if (getArguments() != null) {
      String id = getArguments().getString(TARGET_ID);
      target = System.getTargetById(id);
    }
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container,
                           Bundle savedInstanceState) {
    return inflater.inflate(R.layout.target_detail, container, false);
  }

  @Override
  public void onAttach(Context context) {
    super.onAttach(context);
    if (context instanceof OnFragmentInteractionListener) {
      listener = (OnFragmentInteractionListener) context;
    } else {
      throw new RuntimeException(context.toString()
              + " must implement OnFragmentInteractionListener");
    }
  }

  @Override
  public void onDetach() {
    super.onDetach();
    listener = null;
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    ImageView thumb = (ImageView) view.findViewById(R.id.imageView);
    TextView ip = (TextView) view.findViewById(R.id.ipTextView);
    TextView mac = (TextView) view.findViewById(R.id.macTextView);
    TextView hostname = (TextView) view.findViewById(R.id.hostnameTextView);
    TabLayout tabLayout = (TabLayout) view.findViewById(android.R.id.tabhost);
    final ViewPager viewPager = (ViewPager) view.findViewById(R.id.viewpager);

    TargetPagerAdapter pagerAdapter = new TargetPagerAdapter(getFragmentManager(), target);

    tabLayout.addTab(tabLayout.newTab().setText("Actions"));

    if(pagerAdapter.getCount() > 1) {
      tabLayout.addTab(tabLayout.newTab().setText("Ports"));
    }

    tabLayout.setTabGravity(TabLayout.GRAVITY_FILL);

    viewPager.setAdapter(pagerAdapter);
    viewPager.addOnPageChangeListener(new TabLayout.TabLayoutOnPageChangeListener(tabLayout));

    tabLayout.setOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
      @Override
      public void onTabSelected(TabLayout.Tab tab) {
        viewPager.setCurrentItem(tab.getPosition());
      }

      @Override
      public void onTabUnselected(TabLayout.Tab tab) {
      }

      @Override
      public void onTabReselected(TabLayout.Tab tab) {
      }
    });

    int thumbId = target.getDrawableResourceId();
    Drawable drawable = ContextCompat.getDrawable(getContext(), thumbId);
    thumb.setImageDrawable(drawable);

    ip.setText("IP: " + target.getDisplayAddress());

    if(target.getType() == Target.Type.ENDPOINT) {
      mac.setText("MAC: " + target.getEndpoint().getHardwareAsString());
      mac.setVisibility(View.VISIBLE);
    } else {
      mac.setVisibility(View.INVISIBLE);
    }

    String hname = target.getHostname();

    if(hname != null && !hname.isEmpty()) {
      hostname.setText("NAME: " + hname);
      hostname.setVisibility(View.VISIBLE);
    } else {
      hostname.setVisibility(View.INVISIBLE);
    }
  }

  public void onPluginChosen(Plugin plugin) {
    if (listener != null) {
      listener.onPluginChosen(plugin);
    }
  }

  @Override
  public String toString() {
    return "TargetDetail: " + target.getIdentifier();
  }

  public interface OnFragmentInteractionListener extends PluginList.OnFragmentInteractionListener {
    //void onPortChosen(Port port);
    //and so on
  }
}
