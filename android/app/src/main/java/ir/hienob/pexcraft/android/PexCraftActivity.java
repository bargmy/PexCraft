package ir.hienob.pexcraft.android;

import android.os.Bundle;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class PexCraftActivity extends SDLActivity {
    private static final String TAG = "PexCraftAndroid";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        copyBundledGameAssets();
        super.onCreate(savedInstanceState);
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }

    private void copyBundledGameAssets() {
        File outRoot = new File(getFilesDir(), "assets");
        File stamp = new File(outRoot, ".pexcraft-assets-v1");
        if (stamp.exists()) return;
        if (!outRoot.exists() && !outRoot.mkdirs()) {
            Log.w(TAG, "Could not create asset output dir: " + outRoot);
            return;
        }
        try {
            copyAssetDir("assets", outRoot);
            if (!stamp.exists()) stamp.createNewFile();
        } catch (IOException e) {
            Log.e(TAG, "Failed to copy bundled assets", e);
        }
    }

    private void copyAssetDir(String assetPath, File outDir) throws IOException {
        String[] children = getAssets().list(assetPath);
        if (children == null || children.length == 0) {
            copyAssetFile(assetPath, outDir);
            return;
        }
        if (!outDir.exists() && !outDir.mkdirs()) {
            throw new IOException("Could not create " + outDir);
        }
        for (String child : children) {
            String childAssetPath = assetPath + "/" + child;
            File childOut = new File(outDir, child);
            String[] grandChildren = getAssets().list(childAssetPath);
            if (grandChildren != null && grandChildren.length > 0) copyAssetDir(childAssetPath, childOut);
            else copyAssetFile(childAssetPath, childOut);
        }
    }

    private void copyAssetFile(String assetPath, File outFile) throws IOException {
        File parent = outFile.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) {
            throw new IOException("Could not create " + parent);
        }
        try (InputStream in = getAssets().open(assetPath); OutputStream out = new FileOutputStream(outFile)) {
            byte[] buf = new byte[16 * 1024];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        }
    }
}
