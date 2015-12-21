package org.csploit.android.gui.adapters;

import android.app.Activity;
import android.content.Context;
import android.support.annotation.LayoutRes;
import android.support.annotation.NonNull;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;

import org.csploit.android.R;
import org.csploit.android.helpers.GUIHelper;

import java.util.List;

/**
 * A list adapter that show data from a live list that change
 */
public abstract class BaseLiveListAdapter<T, H> extends BaseAdapter {

  private final Activity activity;
  private final LayoutInflater inflater;
  private final boolean dark;
  private final Class<H> holderClass;
  private List<T> list;
  private ListView lv;

  public BaseLiveListAdapter(@NonNull Activity activity, @NonNull Class<H> holderClass) {
    this.activity = activity;
    dark = GUIHelper.isDarkThemeEnabled();
    this.holderClass = holderClass;
    inflater = (LayoutInflater) activity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent) {
    View row = convertView;
    H holder;
    Object tag = row != null ? row.getTag() : null;

    if (tag == null || !(holderClass.isInstance(tag))) {
      row = inflater.inflate(getItemLayout(), parent, false);

      onRowCreated(row);

      holder = createHolder(row);
      row.setTag(holder);
    } else {
      holder = (H) tag;
    }

    final T item = getList().get(position);

    updateView(item, holder);

    return row;
  }

  @Override
  public int getCount() {
    return getList().size();
  }

  @Override
  public Object getItem(int position) {
    return getList().get(position);
  }

  @Override
  public long getItemId(int position) {
    return 0;
  }

  protected LayoutInflater getInflater() {
    return inflater;
  }

  protected void onRowCreated(View row) {
    if (dark)
      row.setBackgroundResource(R.drawable.card_background_dark);
  }

  protected void updateWholeList() {
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        list = getListCopy();
        notifyDataSetChanged();
      }
    });
  }

  protected void updateOneItem(final T item) {
    if (lv == null) {
      updateWholeList();
      return;
    }

    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        List<T> l = getList();
        int start = lv.getFirstVisiblePosition();
        int end = Math.min(lv.getLastVisiblePosition(), l.size());
        for (int i = start; i <= end; i++) {
          if (item == l.get(i)) {
            View view = lv.getChildAt(i - start);
            getView(i, view, lv);
            break;
          }
        }
      }
    });
  }

  protected boolean isDark() {
    return dark;
  }

  protected Activity getActivity() {
    return activity;
  }

  protected List<T> getList() {
    if(list == null)
      list = getListCopy();
    return list;
  }

  protected abstract List<T> getListCopy();

  protected abstract @LayoutRes int getItemLayout();

  protected abstract H createHolder(View row);

  protected abstract void updateView(T item, H holder);

  public void setListView(ListView lv) {
    this.lv = lv;
  }
}
