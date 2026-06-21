package ir.hienob.pexcraft.tv;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class TvProgramsReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        TvChannelPublisher.publishDefaultChannel(context);
    }
}
