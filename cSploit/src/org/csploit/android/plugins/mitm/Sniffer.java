/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *
 * dSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.csploit.android.plugins.mitm;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import org.csploit.android.ActionActivity;
import org.csploit.android.R;
import org.csploit.android.core.Child;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.ConfirmDialog;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.net.Target;
import org.csploit.android.plugins.mitm.SpoofSession.OnSessionReadyListener;
import org.csploit.android.tools.TcpDump;

import java.io.File;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Collections;

public class Sniffer extends AppCompatActivity implements AdapterView.OnItemClickListener
{
  private static final String[] SORT = {
    "Bandwidth ↓",
    "Bandwidth ↑",
    "Total ↓",
    "Total ↑",
    "Activity ↓",
    "Activity ↑",
  };

  private static final String PCAP_FILTER = "not '(src host localhost or dst host localhost or arp)'";

  private ToggleButton mSniffToggleButton = null;
  private Spinner mSortSpinner = null;
  private int mSortType = 0;
  private ProgressBar mSniffProgress = null;
  private ListView mListView = null;
  private StatListAdapter mAdapter = null;
  private boolean mRunning = false;
  private int mSampleTime = 1000; // sample every second
  private SpoofSession mSpoofSession = null;
  private boolean mDumpToFile = false;
  private String mPcapFileName = null;
  private Child mTcpdumpProcess = null;

  public class AddressStats implements Comparable<AddressStats>{
    public String mAddress = "";
    public long mBytes = 0;
    public long mBandwidth = 0;
    public long mSampledTime = 0;
    public long mSampledBytes = 0;

    public AddressStats(String address){
      mAddress = address;
      mBytes = 0;
      mBandwidth = 0;
      mSampledTime = 0;
      mSampledBytes = 0;
    }

    @Override
    public int compareTo(AddressStats stats){
      int[] cmp;
      double va,vb;

      switch(getSortType()){
        default:
        case 0:
          cmp = new int[]{-1, 1, 0};
          va = mBandwidth;
          vb = stats.mBandwidth;
          break;
        case 1:
          cmp = new int[]{1, -1, 0};
          va = mBandwidth;
          vb = stats.mBandwidth;
          break;
        case 2:
          cmp = new int[]{-1, 1, 0};
          va = mBytes;
          vb = stats.mBytes;
          break;
        case 3:
          cmp = new int[]{1, -1, 0};
          va = mBytes;
          vb = stats.mBytes;
          break;
        case 4:
          cmp = new int[]{-1, 1, 0};
          va = mSampledTime;
          vb = stats.mSampledTime;
          break;
        case 5:
          cmp = new int[]{1, -1, 0};
          va = mSampledTime;
          vb = stats.mSampledTime;
          break;
      }

      if(va > vb)
        return cmp[0];

      else if(va < vb)
        return cmp[1];

      else
        return cmp[2];
    }
  }

  public class StatListAdapter extends ArrayAdapter<AddressStats> {
    private int mLayoutId = 0;
    private final ArrayList<AddressStats> mStats;

    public class StatsHolder {
      TextView address;
      TextView description;
    }

    public StatListAdapter(int layoutId) {
      super(Sniffer.this, layoutId);

      mLayoutId = layoutId;
      mStats = new ArrayList<>();
    }

    public AddressStats getStats(String address) {
      synchronized (mStats) {
        for (AddressStats stats : mStats) {
          if (stats.mAddress.equals(address))
            return stats;
        }
      }

      return null;
    }

    public synchronized void addStats(AddressStats stats) {
      boolean found = false;

      synchronized (mStats) {

        for (AddressStats sstats : mStats) {
          if (sstats.mAddress.equals(stats.mAddress)) {
            sstats.mBytes = stats.mBytes;
            sstats.mBandwidth = stats.mBandwidth;
            sstats.mSampledTime = stats.mSampledTime;
            sstats.mSampledBytes = stats.mSampledBytes;

            found = true;
            break;
          }
        }

        if (!found)
          mStats.add(stats);

        Collections.sort(mStats);
      }
    }

    private synchronized AddressStats getByPosition(int position) {
      synchronized (mStats) {
        return mStats.get(position);
      }
    }

    @Override
    public int getCount(){
      synchronized (mStats) {
        return mStats.size();
      }
    }

    private String formatSize(long size){
      if(size < 1024)
        return size + " B";

      else if(size < (1024 * 1024))
        return (size / 1024) + " KB";

      else if(size < (1024 * 1024 * 1024))
        return (size / (1024 * 1024)) + " MB";

      else
        return (size / (1024 * 1024 * 1024)) + " GB";
    }

    private String formatSpeed(long speed){
      if(speed < 1024)
        return speed + " B/s";

      else if(speed < (1024 * 1024))
        return (speed / 1024) + " KB/s";

      else if(speed < (1024 * 1024 * 1024))
        return (speed / (1024 * 1024)) + " MB/s";

      else
        return (speed / (1024 * 1024 * 1024)) + " GB/s";
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent){
      View row = convertView;
      StatsHolder holder = null;

      if(row == null){
        LayoutInflater inflater = (LayoutInflater) Sniffer.this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        row = inflater.inflate(mLayoutId, parent, false);

        holder = new StatsHolder();

        holder.address = (TextView) row.findViewById(R.id.statAddress);
        holder.description = (TextView) row.findViewById(R.id.statDescription);

        row.setTag(holder);
      } else{
        holder = (StatsHolder) row.getTag();
      }

      AddressStats stats = getByPosition(position);
      Target target = System.getTargetByAddress(stats.mAddress);

      if(target != null && target.hasAlias())
        holder.address.setText
          (
            Html.fromHtml
              (
                "<b>" + target.getAlias() + "</b> <small>( " + target.getDisplayAddress() + " )</small>"
              )
          );
      else
        holder.address.setText(stats.mAddress);

      holder.description.setText
        (
          Html.fromHtml
            (
              "<b>BANDWIDTH</b>: " + formatSpeed(stats.mBandwidth) + " | <b>TOTAL</b> " + formatSize(stats.mBytes)
            )
        );

      return row;
    }
  }

  public void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);
    SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
  	Boolean isDark = themePrefs.getBoolean("isDark", false);

    if (isDark)
      setTheme(R.style.DarkTheme);
    else
      setTheme(R.style.AppTheme);

    setTitle(System.getCurrentTarget() + " > MITM > Sniffer");
    setContentView(R.layout.plugin_mitm_sniffer);
    getSupportActionBar().setDisplayHomeAsUpEnabled(true);

    mSniffToggleButton = (ToggleButton) findViewById(R.id.sniffToggleButton);
    mSniffProgress = (ProgressBar) findViewById(R.id.sniffActivity);
    mSortSpinner = (Spinner) findViewById(R.id.sortSpinner);
    mListView = (ListView) findViewById(R.id.listView);
    mAdapter = new StatListAdapter(R.layout.plugin_mitm_sniffer_list_item);
    mSampleTime = (int)(Double.parseDouble(System.getSettings().getString("PREF_SNIFFER_SAMPLE_TIME", "1.0")) * 1000);
    mSpoofSession = new SpoofSession(false, false, null, null);

    mSortSpinner.setAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, SORT));
    mSortSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
      public void onItemSelected(AdapterView<?> adapter, View view, int position, long id) {
        mSortType = position;
      }

      public void onNothingSelected(AdapterView<?> arg0) {
      }
    });

    mListView.setAdapter(mAdapter);

    mListView.setOnItemClickListener(this);

    mSniffToggleButton.setOnClickListener(new View.OnClickListener(){
      @Override
      public void onClick(View v){
        if(mRunning){
          setStoppedState();
        } else{
          setStartedState();
        }
      }
    }
    );

    new ConfirmDialog( getString(R.string.file_output), getString(R.string.question_save_to_pcap), this, new ConfirmDialog.ConfirmDialogListener(){
      @Override
      public void onConfirm(){
        mDumpToFile = true;
        mPcapFileName = (new File(System.getStoragePath(), "csploit-sniff-" + java.lang.System.currentTimeMillis() + ".pcap")).getAbsolutePath();
      }

      @Override
      public void onCancel(){
        mDumpToFile = false;
        mPcapFileName = null;
      }
    }).show();
  }

  @Override
  public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
    String address = mAdapter.getByPosition(position).mAddress;
    final Target t = System.getTargetByAddress(address);

    if (t == null)
      return;

    new ConfirmDialog(getString(R.string.mitm_ss_select_target_title),
            String.format(getString(R.string.mitm_ss_select_target_prompt), address),
            Sniffer.this, new ConfirmDialog.ConfirmDialogListener() {
      @Override
      public void onConfirm() {
        System.setCurrentTarget(t);

        setStoppedState();

        Toast.makeText(Sniffer.this,
                getString(R.string.selected_) + System.getCurrentTarget(),
                Toast.LENGTH_SHORT).show();

        startActivity(new Intent(Sniffer.this,
                ActionActivity.class));

        overridePendingTransition(R.anim.slide_in_left,
                R.anim.slide_out_left);
      }

      @Override
      public void onCancel() {
      }
    }).show();
  }

  public synchronized int getSortType(){
    return mSortType;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item){
    switch(item.getItemId()){
      case android.R.id.home:

        onBackPressed();

        return true;

      default:
        return super.onOptionsItemSelected(item);
    }
  }

  private void setStoppedState(){
    if(mTcpdumpProcess != null) {
      mTcpdumpProcess.kill();
      mTcpdumpProcess = null;
    }
    Sniffer.this.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        mSpoofSession.stop();

        mSniffProgress.setVisibility(View.INVISIBLE);

        mRunning = false;
        mSniffToggleButton.setChecked(false);
      }
    });
  }

  private void setSpoofErrorState(final String error){
    Sniffer.this.runOnUiThread(new Runnable(){
      @Override
      public void run(){
        new ErrorDialog("Error", error, Sniffer.this).show();
        setStoppedState();
      }
    });
  }

  private void setStartedState(){

    if(mDumpToFile)
      Toast.makeText(Sniffer.this, getString(R.string.dumping_traffic_to) + mPcapFileName, Toast.LENGTH_SHORT).show();

    try {
      mSpoofSession.start(new OnSessionReadyListener(){
        @Override
        public void onError(String error, int resId){
          error = error == null ? getString(resId) : error;
          setSpoofErrorState(error);
        }

        @Override
        public void onSessionReady(){
          if(mTcpdumpProcess!=null) {
            mTcpdumpProcess.kill();
          }

          try {
            mRunning = true;
            mSniffProgress.setVisibility(View.VISIBLE);

            mTcpdumpProcess = System.getTools().tcpDump.sniff(PCAP_FILTER, mPcapFileName, new TcpDump.TcpDumpReceiver() {

              @Override
              public void onPacket(InetAddress src, InetAddress dst, int len) {
              long now = java.lang.System.currentTimeMillis();
              long deltat;
              AddressStats stats = null;
              String stringAddress = null;

              if (System.getNetwork().isInternal(src)) {
                stringAddress = src.getHostAddress();
                stats = mAdapter.getStats(stringAddress);
              } else if (System.getNetwork().isInternal(dst)) {
                stringAddress = dst.getHostAddress();
                stats = mAdapter.getStats(stringAddress);
              }

              if (stats == null) {
                if(stringAddress==null)
                  return;
                stats = new AddressStats(stringAddress);
                stats.mBytes = len;
                stats.mSampledTime = now;
              } else {
                stats.mBytes += len;

                deltat = (now - stats.mSampledTime);

                if (deltat >= mSampleTime) {
                  stats.mBandwidth = (stats.mBytes - stats.mSampledBytes) / deltat;
                  stats.mSampledTime = java.lang.System.currentTimeMillis();
                  stats.mSampledBytes = stats.mBytes;
                }
              }

              final AddressStats fstats = stats;
              Sniffer.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                  mAdapter.addStats(fstats);
                  mAdapter.notifyDataSetChanged();
                }
              });
              }
            });
          } catch( ChildManager.ChildNotStartedException e ) {
            Sniffer.this.runOnUiThread( new Runnable() {
              @Override
              public void run() {
                Toast.makeText(Sniffer.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
                setStoppedState();
              }
            });
          }
        }
      });
    } catch (ChildManager.ChildNotStartedException e) {
      setStoppedState();
    }
  }

  @Override
  public void onBackPressed(){
    setStoppedState();
    super.onBackPressed();
    overridePendingTransition(R.anim.fadeout, R.anim.fadein);
  }
}