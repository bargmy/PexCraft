package ir.hienob.pexcraft.tv;

import android.content.Context;
import android.content.ContentUris;
import android.content.SharedPreferences;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Build;
import android.util.Log;

import androidx.tvprovider.media.tv.Channel;
import androidx.tvprovider.media.tv.ChannelLogoUtils;
import androidx.tvprovider.media.tv.PreviewProgram;
import androidx.tvprovider.media.tv.TvContractCompat;

public final class TvChannelPublisher {
    private static final String TAG = "PexCraftTV";
    private static final String PREFS = "pexcraft_tv_provider";
    private static final String CHANNEL_ID = "channel_id";
    private static final String PROGRAM_ID = "program_id";

    private TvChannelPublisher() {}

    public static void publishDefaultChannel(Context context) {
        if (Build.VERSION.SDK_INT < 26) return;
        try {
            SharedPreferences prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
            long channelId = prefs.getLong(CHANNEL_ID, -1L);
            if (channelId <= 0) {
                Uri appLink = Uri.parse("pexcraft://play/home");
                Channel channel = new Channel.Builder()
                        .setType(TvContractCompat.Channels.TYPE_PREVIEW)
                        .setDisplayName("PexCraft")
                        .setDescription("Play PexCraft on Android TV")
                        .setAppLinkIntentUri(appLink)
                        .build();
                Uri channelUri = context.getContentResolver().insert(
                        TvContractCompat.Channels.CONTENT_URI,
                        channel.toContentValues()
                );
                if (channelUri != null) {
                    channelId = ContentUris.parseId(channelUri);
                    ChannelLogoUtils.storeChannelLogo(context, channelId,
                            BitmapFactory.decodeResource(context.getResources(), R.drawable.ic_launcher));
                    TvContractCompat.requestChannelBrowsable(context, channelId);
                    prefs.edit().putLong(CHANNEL_ID, channelId).apply();
                }
            }
            if (channelId > 0 && prefs.getLong(PROGRAM_ID, -1L) <= 0) {
                Uri poster = Uri.parse("android.resource://" + context.getPackageName() + "/drawable/tv_banner");
                Uri play = Uri.parse("pexcraft://play/home");
                PreviewProgram program = new PreviewProgram.Builder()
                        .setChannelId(channelId)
                        .setType(TvContractCompat.PreviewPrograms.TYPE_GAME)
                        .setTitle("Play PexCraft")
                        .setDescription("Open PexCraft and continue your world")
                        .setPosterArtUri(poster)
                        .setPosterArtAspectRatio(TvContractCompat.PreviewPrograms.ASPECT_RATIO_16_9)
                        .setIntentUri(play)
                        .setInternalProviderId("pexcraft_play")
                        .build();
                Uri programUri = context.getContentResolver().insert(
                        TvContractCompat.PreviewPrograms.CONTENT_URI,
                        program.toContentValues()
                );
                if (programUri != null) {
                    prefs.edit().putLong(PROGRAM_ID, ContentUris.parseId(programUri)).apply();
                }
            }
        } catch (Throwable t) {
            Log.w(TAG, "Could not publish Android TV channel", t);
        }
    }
}
