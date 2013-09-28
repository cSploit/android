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
package it.evilsocket.dsploit.plugins.mitm;

import android.content.Context;
import android.os.Bundle;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
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

import com.actionbarsherlock.app.SherlockActivity;
import com.actionbarsherlock.view.MenuItem;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.plugins.mitm.SpoofSession.OnSessionReadyListener;

public class Sniffer extends SherlockActivity{
  private static final String TAG = "SNIFFER";
  private static final String[] SORT = {
    "Bandwidth ↓",
    "Bandwidth ↑",
    "Total ↓",
    "Total ↑",
    "Activity ↓",
    "Activity ↑",
  };

  private static final String PCAP_FILTER = "not '(src host localhost or dst host localhost or arp)'";
  private static final Pattern PARSER = Pattern.compile("^.+length\\s+(\\d+)\\)\\s+([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3})\\.[^\\s]+\\s+>\\s+([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3})\\.[^\\:]+:.+", Pattern.CASE_INSENSITIVE);

  private ToggleButton mSniffToggleButton = null;
  private Spinner mSortSpinner = null;
  private int mSortType = 0;
  private ProgressBar mSniffProgress = null;
  private ListView mListView = null;
  private StatListAdapter mAdapter = null;
  private boolean mRunning = false;
  private double mSampleTime = 1.0;
  private SpoofSession mSpoofSession = null;
  private boolean mDumpToFile = false;
  private String mPcapFileName = null;

  public class AddressStats implements Comparable<AddressStats>{
    public String mAddress = "";
    public int mPackets = 0;
    public double mBandwidth = 0;
    public long mSampledTime = 0;
    public int mSampledSize = 0;

    public AddressStats(String address){
      mAddress = address;
      mPackets = 0;
      mBandwidth = 0;
      mSampledTime = 0;
      mSampledSize = 0;
    }

    @Override
    public int compareTo(AddressStats stats){
      int[] cmp = null;
      double va = 0,
        vb = 0;

      switch(getSortType()){
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
          va = mPackets;
          vb = stats.mPackets;
          break;
        case 3:
          cmp = new int[]{1, -1, 0};
          va = mPackets;
          vb = stats.mPackets;
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

        default:
          cmp = new int[]{-1, 1, 0};
          va = mBandwidth;
          vb = stats.mBandwidth;
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

  public class StatListAdapter extends ArrayAdapter<AddressStats>{
    private int mLayoutId = 0;
    private ArrayList<AddressStats> mStats = null;

    public class StatsHolder{
      TextView address;
      TextView description;
    }

    public StatListAdapter(int layoutId){
      super(Sniffer.this, layoutId);

      mLayoutId = layoutId;
      mStats = new ArrayList<AddressStats>();
    }

    public AddressStats getStats(String address){
      for(AddressStats stats : mStats){
        if(stats.mAddress.equals(address))
          return stats;
      }

      return null;
    }

    public synchronized void addStats(AddressStats stats){
      boolean found = false;

      for(AddressStats sstats : mStats){
        if(sstats.mAddress.equals(stats.mAddress)){
          sstats.mPackets = stats.mPackets;
          sstats.mBandwidth = stats.mBandwidth;
          sstats.mSampledTime = stats.mSampledTime;
          sstats.mSampledSize = stats.mSampledSize;

          found = true;
          break;
        }
      }

      if(found == false)
        mStats.add(stats);

      Collections.sort(mStats);
    }

    private synchronized AddressStats getByPosition(int position){
      return mStats.get(position);
    }

    @Override
    public int getCount(){
      return mStats.size();
    }

    private String formatSize(int size){
      if(size < 1024)
        return size + " B";

      else if(size < (1024 * 1024))
        return (size / 1024) + " KB";

      else if(size < (1024 * 1024 * 1024))
        return (size / (1024 * 1024)) + " MB";

      else
        return (size / (1024 * 1024 * 1024)) + " GB";
    }

    private String formatSpeed(int speed){
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
              "<b>BANDWIDTH</b>: " + formatSpeed((int) stats.mBandwidth) + " | <b>TOTAL</b> " + formatSize(stats.mPackets)
            )
        );

      return row;
    }
  }

  public void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);
    setTitle(System.getCurrentTarget() + " > MITM > Sniffer");
    setContentView(R.layout.plugin_mitm_sniffer);
    getSupportActionBar().setDisplayHomeAsUpEnabled(true);

    mSniffToggleButton = (ToggleButton) findViewById(R.id.sniffToggleButton);
    mSniffProgress = (ProgressBar) findViewById(R.id.sniffActivity);
    mSortSpinner = (Spinner) findViewById(R.id.sortSpinner);
    mListView = (ListView) findViewById(R.id.listView);
    mAdapter = new StatListAdapter(R.layout.plugin_mitm_sniffer_list_item);
    mSampleTime = Double.parseDouble(System.getSettings().getString("PREF_SNIFFER_SAMPLE_TIME", "1.0"));
    mSpoofSession = new SpoofSession(false, false, null, null);

    mSortSpinner.setAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, SORT));
    mSortSpinner.setOnItemSelectedListener(new OnItemSelectedListener(){
      public void onItemSelected(AdapterView<?> adapter, View view, int position, long id){
        mSortType = position;
      }

      public void onNothingSelected(AdapterView<?> arg0){
      }
    });

    mListView.setAdapter(mAdapter);

    mSniffToggleButton.setOnClickListener(new OnClickListener(){
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

    new ConfirmDialog( getString(R.string.file_output), getString(R.string.question_save_to_pcap), this, new ConfirmDialogListener(){
      @Override
      public void onConfirm(){
        mDumpToFile = true;
        mPcapFileName = (new File(System.getStoragePath(), "dsploit-sniff-" + java.lang.System.currentTimeMillis() + ".pcap")).getAbsolutePath();
      }

      @Override
      public void onCancel(){
        mDumpToFile = false;
        mPcapFileName = null;
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
    mSpoofSession.stop();

    System.getTcpDump().kill();

    mSniffProgress.setVisibility(View.INVISIBLE);

    mRunning = false;
    mSniffToggleButton.setChecked(false);
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
    // never say never :)
    System.getTcpDump().kill();

    if(mDumpToFile)
      Toast.makeText(Sniffer.this, getString(R.string.dumping_traffic_to) + mPcapFileName, Toast.LENGTH_SHORT).show();

    mSpoofSession.start(new OnSessionReadyListener(){
      @Override
      public void onError(String error){
        setSpoofErrorState(error);
      }

      @Override
      public void onSessionReady(){

        System.getTcpDump().sniff(PCAP_FILTER, mPcapFileName, new OutputReceiver(){
          @Override
          public void onStart(String command){
          }

          @Override
          public void onNewLine(String line){

            try{
              Matcher matcher = PARSER.matcher(line.trim());

              if(matcher != null && matcher.find()){
                String length = matcher.group(1),
                  source = matcher.group(2),
                  dest = matcher.group(3);
                int ilength = Integer.parseInt(length);
                long now = java.lang.System.currentTimeMillis();
                double deltat = 0.0;
                AddressStats stats = null;

                if(System.getNetwork().isInternal(source)){
                  stats = mAdapter.getStats(source);
                } else if(System.getNetwork().isInternal(dest)){
                  source = dest;
                  stats = mAdapter.getStats(dest);
                }

                if(stats == null){
                  stats = new AddressStats(source);
                  stats.mPackets = ilength;
                  stats.mSampledTime = now;
                } else{
                  stats.mPackets += ilength;

                  deltat = (now - stats.mSampledTime) / 1000.0;

                  if(deltat >= mSampleTime){
                    stats.mBandwidth = (stats.mPackets - stats.mSampledSize) / deltat;
                    stats.mSampledTime = java.lang.System.currentTimeMillis();
                    stats.mSampledSize = stats.mPackets;
                  }
                }

                final AddressStats fstats = stats;
                Sniffer.this.runOnUiThread(new Runnable(){
                  @Override
                  public void run(){
                    mAdapter.addStats(fstats);
                    mAdapter.notifyDataSetChanged();
                  }
                });
              }
            } catch(Exception e){
              System.errorLogging(TAG, e);
            }
          }

          @Override
          public void onEnd(int exitCode){
          }
        }).start();
      }
    });

    mSniffProgress.setVisibility(View.VISIBLE);
    mRunning = true;
  }

  @Override
  public void onBackPressed(){
    setStoppedState();
    super.onBackPressed();
    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
  }
}