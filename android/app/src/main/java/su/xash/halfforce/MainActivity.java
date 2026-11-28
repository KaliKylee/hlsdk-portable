package su.xash.halfforce;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.ClipDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.RippleDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.StatFs;
import android.os.SystemClock;
import android.provider.Settings;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipException;
import java.util.zip.ZipFile;

public class MainActivity extends Activity {

    private static final String GAMEDIR = "half-force-v2.0";
    private static final String PREFS_NAME = "half_force_settings";
    private static final String PREF_GAME_ARGS = "game_arguments";
    private static final String DEFAULT_GAME_ARGS = "-console -log";

    private EditText gameArgsInput;

    private static final String MOD_VERSION = "2.0";

    private static final String URL_GITHUB = "https://github.com/KaliKylee/hlsdk-portable/tree/half-force";
    private static final String URL_YOUTUBE = "https://youtube.com/@kalikyle3";
    private static final String URL_DISCORD = "https://discord.gg/fUnXU4RZgn";
    private static final String URL_ENGINE = "https://github.com/FWGS/xash3d-fwgs/releases/tag/continuous";

    private static final String[][] WALLETS = {
        {"USDT  \u2022  Polygon", "0xe3c825c7eF4a3609E66b5D92C671d5C23dE78368"},
        {"USDT  \u2022  Ethereum", "0xe3c825c7eF4a3609E66b5D92C671d5C23dE78368"},
        {"USDT  \u2022  Solana", "G8br5Hgu9jSV1d84g7LfeNMBDJYRwwhntBBebA52QdLk"},
    };

    private static final int BG = 0xFF0F1011;
    private static final int PANEL = 0xFF171A18;
    private static final int RED = 0xFFC93C3C;
    private static final int RED_DARK = 0xFF6E2222;
    private static final int GREEN = 0xFF3F9A63;
    private static final int GREEN_DEEP = 0xFF1F5A38;

    private static final String ACTION_MOD = "su.xash.engine.MOD";

    // DOWNLOADS

    private static final String XASH_DIR = "/storage/emulated/0/xash";

    private static final String URL_VALVE = "https://www.dropbox.com/scl/fi/917y7zdtoycwn0grjt28i/valve.zip?rlkey=9dexlaia9mluk5ull27omf2u2&st=cpr4epvh&dl=0";
    private static final String URL_HALFFORCE = "https://www.dropbox.com/scl/fi/3vtwi852mhtyqrea9t6va/half-force-v2.0.zip?rlkey=86jsvy8gp6capzg2uz3m0fffb&st=ekq1snjh&dl=0";

    private static final int REQ_STORAGE = 4021;

    private static final int ST_IDLE = 0;
    private static final int ST_DOWNLOADING = 1;
    private static final int ST_INSTALLING = 2;

    private final Handler ui = new Handler(Looper.getMainLooper());
    private DownloadItem[] downloads;
    private DownloadItem pendingItem;
    private AlertDialog downloadsDialog;

    // IDIOMA (0 = portugues, 1 = ingles, 2 = espanhol)

    private static final String PREF_LANG = "language";
    private static final String[] LANG_NAMES = {"Portugu\u00eas", "English", "Espa\u00f1ol"};
    private volatile int lang = 1;

    private String tr(String pt, String en, String es) {
        if (lang == 0) {
            return pt;
        }

        return lang == 2 ? es : en;
    }

    private int systemLang() {
        String l = java.util.Locale.getDefault().getLanguage();

        if ("pt".equals(l)) {
            return 0;
        }

        if ("es".equals(l)) {
            return 2;
        }

        return 1;
    }

    private void loadLang() {
        int sys = systemLang();
        int saved = getSharedPreferences(PREFS_NAME, MODE_PRIVATE).getInt(PREF_LANG, sys);
        lang = (saved >= 0 && saved <= 2) ? saved : sys;
    }

    private void setLanguage(int l) {
        lang = l;

        getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                .edit()
                .putInt(PREF_LANG, l)
                .apply();

        if (downloadsDialog != null) {
            downloadsDialog.dismiss();
            downloadsDialog = null;
        }

        if (gameArgsInput != null) {
            getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                    .edit()
                    .putString(PREF_GAME_ARGS, gameArgsInput.getText().toString().trim())
                    .apply();
        }

        setContentView(buildUi());
        setupDownloadsButton();
    }

    private void showLanguageDialog() {
        LinearLayout lb = new LinearLayout(this);
        lb.setOrientation(LinearLayout.VERTICAL);
        lb.setPadding(dp(20), dp(20), dp(20), dp(12));

        TextView title = text(tr("IDIOMA", "LANGUAGE", "IDIOMA"), 18, RED, true);
        title.setLetterSpacing(0.15f);
        lb.addView(title, wrap());

        final AlertDialog[] ref = new AlertDialog[1];

        for (int i = 0; i < LANG_NAMES.length; i++) {
            final int idx = i;
            boolean sel = (i == lang);

            TextView opt = text(LANG_NAMES[i], 15, sel ? RED : GREEN, true);
            opt.setGravity(Gravity.CENTER);
            opt.setBackground(ripple(box(sel ? RED_DARK : PANEL, sel ? RED : GREEN_DEEP, 2, 12)));

            opt.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (ref[0] != null) {
                        ref[0].dismiss();
                    }

                    if (idx != lang) {
                        setLanguage(idx);
                    }
                }
            });

            LinearLayout.LayoutParams optLp = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    dp(48)
            );

            optLp.topMargin = dp(10);
            lb.addView(opt, optLp);
        }

        ref[0] = new AlertDialog.Builder(this)
                .setView(lb)
                .setPositiveButton(tr("Fechar", "Close", "Cerrar"), null)
                .create();

        ref[0].getWindow().setBackgroundDrawable(box(BG, GREEN_DEEP, 2, 18));
        ref[0].show();
        ref[0].getButton(AlertDialog.BUTTON_POSITIVE).setTextColor(GREEN);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        loadLang();

        Intent in = getIntent();

        if (in != null && ACTION_MOD.equals(in.getAction())) {
            launchGame();
            return;
        }

        if (Build.VERSION.SDK_INT >= 21) {
            Window w = getWindow();
            w.setStatusBarColor(BG);
            w.setNavigationBarColor(BG);
        }

        downloads = new DownloadItem[] {
            new DownloadItem(
                    "valve",
                    "Half-Life 1 (Valve Files)",
                    "Arquivos obrigat\u00f3rios para o funcionamento do jogo",
                    URL_VALVE, "valve", "valve.zip"),
            new DownloadItem(
                    "halfforce",
                    "Half-Force v2.0 (Half Force Files)",
                    "Arquivos necess\u00e1rios para o mod",
                    URL_HALFFORCE, GAMEDIR, "half-force-v2.0.zip")
        };

        setContentView(buildUi());
        setupDownloadsButton();
    }

    @Override
    protected void onResume() {
        super.onResume();

        if (downloads == null) {
            return;
        }

        for (DownloadItem d : downloads) {
            d.refreshInstalled();
            d.render();
        }

        if (pendingItem != null) {
            DownloadItem item = pendingItem;
            pendingItem = null;

            if (hasStoragePermission()) {
                item.start();
            }
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        if (downloads == null) {
            return;
        }

        // reconstroi a UI preservando os argumentos digitados e o estado dos downloads
        if (gameArgsInput != null) {
            getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                    .edit()
                    .putString(PREF_GAME_ARGS, gameArgsInput.getText().toString().trim())
                    .apply();
        }

        setContentView(buildUi());
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if (downloadsDialog != null) {
            downloadsDialog.dismiss();
            downloadsDialog = null;
        }

        if (isFinishing() && downloads != null) {
            for (DownloadItem d : downloads) {
                d.cancelled = true;
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode != REQ_STORAGE) {
            return;
        }

        DownloadItem item = pendingItem;
        pendingItem = null;

        if (item != null) {
            if (hasStoragePermission()) {
                item.start();
            } else {
                Toast.makeText(this,
                        tr("Permiss\u00e3o de armazenamento negada. N\u00e3o \u00e9 poss\u00edvel instalar os arquivos.", "Storage permission denied. The files cannot be installed.", "Permiso de almacenamiento denegado. No se pueden instalar los archivos."),
                        Toast.LENGTH_LONG).show();
            }
        }
    }

    // PERMISSAO DE ARMAZENAMENTO

    private boolean hasStoragePermission() {
        if (Build.VERSION.SDK_INT >= 30) {
            return Environment.isExternalStorageManager();
        }

        if (Build.VERSION.SDK_INT >= 23) {
            return checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    == PackageManager.PERMISSION_GRANTED;
        }

        return true;
    }

    private void requestStoragePermission(DownloadItem item) {
        pendingItem = item;

        if (Build.VERSION.SDK_INT >= 30) {
            LinearLayout pbox = new LinearLayout(this);
            pbox.setOrientation(LinearLayout.VERTICAL);
            pbox.setPadding(dp(20), dp(20), dp(20), dp(8));

            TextView ptitle = text(tr("Permiss\u00e3o necess\u00e1ria", "Permission required", "Permiso necesario"), 18, RED, true);
            ptitle.setLetterSpacing(0.1f);
            pbox.addView(ptitle, wrap());

            TextView pmsg = text(
                    tr("Para instalar os arquivos em ", "To install the files in ", "Para instalar los archivos en ") + XASH_DIR + tr(", permita o acesso a todos os arquivos para este app e volte ao launcher.", ", allow access to all files for this app and return to the launcher.", ", permita el acceso a todos los archivos para esta app y vuelva al launcher."),
                    14, 0xFFE8E8E8, false
            );

            LinearLayout.LayoutParams pmsgLp = wrap();
            pmsgLp.topMargin = dp(12);
            pbox.addView(pmsg, pmsgLp);

            AlertDialog dialog = new AlertDialog.Builder(this)
                    .setView(pbox)
                    .setPositiveButton(tr("Abrir configura\u00e7\u00f5es", "Open settings", "Abrir configuraci\u00f3n"), new android.content.DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(android.content.DialogInterface d, int which) {
                            openAllFilesSettings();
                        }
                    })
                    .setNegativeButton(tr("Cancelar", "Cancel", "Cancelar"), new android.content.DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(android.content.DialogInterface d, int which) {
                            pendingItem = null;
                        }
                    })
                    .create();

            dialog.getWindow().setBackgroundDrawable(box(BG, GREEN_DEEP, 2, 18));
            dialog.show();
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setTextColor(GREEN);
            dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setTextColor(RED);
        } else if (Build.VERSION.SDK_INT >= 23) {
            requestPermissions(new String[] {
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
            }, REQ_STORAGE);
        }
    }

    private void openAllFilesSettings() {
        if (Build.VERSION.SDK_INT < 30) {
            return;
        }

        try {
            startActivity(new Intent(
                    Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                    Uri.parse("package:" + getPackageName())));
        } catch (ActivityNotFoundException e) {
            try {
                startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
            } catch (ActivityNotFoundException ex) {
                pendingItem = null;
                Toast.makeText(this,
                        tr("N\u00e3o foi poss\u00edvel abrir as configura\u00e7\u00f5es de permiss\u00e3o", "Could not open the permission settings", "No se pudo abrir la configuraci\u00f3n de permisos"),
                        Toast.LENGTH_LONG).show();
            }
        }
    }

    private void updateKeepScreenOn() {
        boolean busy = false;

        if (downloads != null) {
            for (DownloadItem d : downloads) {
                if (d.state != ST_IDLE) {
                    busy = true;
                }
            }
        }

        if (busy) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        } else {
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
    }

    private void launchGame() {
        String pkg = "su.xash.engine.test";

        try {
            getPackageManager().getPackageInfo(pkg, 0);
        } catch (PackageManager.NameNotFoundException e) {
            try {
                pkg = "su.xash.engine";
                getPackageManager().getPackageInfo(pkg, 0);
            } catch (PackageManager.NameNotFoundException ex) {
                Toast.makeText(this, tr("Instale o Xash3D FWGS para jogar", "Install Xash3D FWGS to play", "Instala Xash3D FWGS para jugar"), Toast.LENGTH_LONG).show();
                openUrl(URL_ENGINE);
                return;
            }
        }

        Intent game = new Intent();
        game.setComponent(new ComponentName(pkg, "su.xash.engine.XashActivity"));
        game.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);

        game.putExtra("gamedir", GAMEDIR);
        game.putExtra("gamelibdir", getApplicationInfo().nativeLibraryDir);
        game.putExtra("package", getPackageName());

        String gameArgs;

        if (gameArgsInput != null) {
            gameArgs = gameArgsInput.getText().toString().trim();

            getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                    .edit()
                    .putString(PREF_GAME_ARGS, gameArgs)
                    .apply();

        } else {
            gameArgs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                    .getString(PREF_GAME_ARGS, DEFAULT_GAME_ARGS);
        }

        if (!gameArgs.isEmpty()) {
            game.putExtra("argv", gameArgs);
        }

        try {
            startActivity(game);
            finish();
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, tr("N\u00e3o foi poss\u00edvel abrir o Xash3D FWGS", "Could not open Xash3D FWGS", "No se pudo abrir Xash3D FWGS"), Toast.LENGTH_LONG).show();
        }
    }

    private void openUrl(String url) {
        try {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, tr("Nenhum app para abrir o link", "No app available to open the link", "Ninguna app para abrir el enlace"), Toast.LENGTH_SHORT).show();
        }
    }

    private View buildUi() {
        FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(BG);

        // evita que o campo de argumentos pegue o foco e abra o teclado sozinho
        root.setFocusableInTouchMode(true);
        root.requestFocus();

        root.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener() {
            @Override
            public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
                v.setPadding(
                        insets.getSystemWindowInsetLeft(),
                        insets.getSystemWindowInsetTop(),
                        insets.getSystemWindowInsetRight(),
                        insets.getSystemWindowInsetBottom()
                );
                return insets;
            }
        });

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setVerticalScrollBarEnabled(false);

        root.addView(scroll, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        ));

        FrameLayout holder = new FrameLayout(this);

        scroll.addView(holder, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        ));

        LinearLayout col = new LinearLayout(this);
        col.setOrientation(LinearLayout.VERTICAL);
        col.setGravity(Gravity.CENTER_HORIZONTAL);
        col.setPadding(dp(20), dp(24), dp(20), dp(24));

        int width = getResources().getDisplayMetrics().widthPixels - dp(16);

        if (width > dp(460)) {
            width = dp(460);
        }

        FrameLayout.LayoutParams colLp = new FrameLayout.LayoutParams(
                width,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                Gravity.CENTER
        );

        holder.addView(col, colLp);

        TextView title = text("HALF-FORCE", 40, RED, true);
        title.setLetterSpacing(0.12f);
        title.setGravity(Gravity.CENTER);
        col.addView(title, wrap());

        TextView version = text(tr("VERS\u00c3O ", "VERSION ", "VERSI\u00d3N ") + MOD_VERSION, 15, GREEN, true);
        version.setLetterSpacing(0.3f);
        version.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams vLp = wrap();
        vLp.topMargin = dp(2);
        col.addView(version, vLp);

        View line = new View(this);
        line.setBackgroundColor(GREEN_DEEP);

        LinearLayout.LayoutParams lineLp = new LinearLayout.LayoutParams(dp(120), dp(2));
        lineLp.topMargin = dp(18);
        lineLp.bottomMargin = dp(26);
        col.addView(line, lineLp);

        TextView play = text("PLAY", 24, RED, true);
        play.setLetterSpacing(0.25f);
        play.setGravity(Gravity.CENTER);
        play.setBackground(ripple(box(RED_DARK, RED, 2, 14)));

        play.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                launchGame();
            }
        });

        LinearLayout.LayoutParams playLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                dp(64)
        );

        col.addView(play, playLp);

        // ARGUMENTOS DE INICIALIZACAO

        TextView argsLabel = text(tr("ARGUMENTOS DE INICIALIZA\u00c7\u00c3O", "STARTUP ARGUMENTS", "ARGUMENTOS DE INICIO"), 12, GREEN, true);
        argsLabel.setLetterSpacing(0.08f);

        LinearLayout.LayoutParams argsLabelLp = wrap();
        argsLabelLp.topMargin = dp(16);
        col.addView(argsLabel, argsLabelLp);

        gameArgsInput = new EditText(this);
        gameArgsInput.setSingleLine(true);
        gameArgsInput.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        gameArgsInput.setTextColor(0xFFE8E8E8);
        gameArgsInput.setHintTextColor(0xFF858585);
        gameArgsInput.setHint("-console -log");

        gameArgsInput.setText(
                getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                        .getString(PREF_GAME_ARGS, DEFAULT_GAME_ARGS)
        );

        gameArgsInput.setPadding(dp(14), dp(12), dp(14), dp(12));
        gameArgsInput.setBackground(box(PANEL, GREEN_DEEP, 1, 10));

        LinearLayout.LayoutParams argsInputLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                dp(50)
        );

        argsInputLp.topMargin = dp(8);
        col.addView(gameArgsInput, argsInputLp);

        TextView argsHint = text(
                tr("Personalize os par\u00e2metros enviados ao Xash3D FWGS.", "Customize the parameters sent to Xash3D FWGS.", "Personaliza los par\u00e1metros enviados a Xash3D FWGS."),
                11, GREEN, false
        );

        LinearLayout.LayoutParams argsHintLp = wrap();
        argsHintLp.topMargin = dp(5);
        col.addView(argsHint, argsHintLp);

        // LINKS

        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);

        LinearLayout.LayoutParams rowLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        );

        rowLp.topMargin = dp(18);
        col.addView(row, rowLp);

        row.addView(
                linkButton(new PolyIcon(GITHUB, GREEN, dp(30)), "GitHub", URL_GITHUB),
                cell(false)
        );

        row.addView(
                linkButton(new YouTubeIcon(GREEN, dp(30)), "YouTube", URL_YOUTUBE),
                cell(true)
        );

        row.addView(
                linkButton(new PolyIcon(DISCORD, GREEN, dp(30)), "Discord", URL_DISCORD),
                cell(true)
        );

        // APOIE-ME

        TextView support = text(tr("APOIE-ME", "SUPPORT ME", "AP\u00d3YAME"), 16, GREEN, true);
        support.setLetterSpacing(0.2f);
        support.setGravity(Gravity.CENTER);
        support.setBackground(ripple(box(PANEL, GREEN_DEEP, 2, 14)));

        support.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showSupportDialog();
            }
        });

        LinearLayout.LayoutParams supLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                dp(52)
        );

        supLp.topMargin = dp(12);
        col.addView(support, supLp);

        TextView langBtn = text(tr("IDIOMA", "LANGUAGE", "IDIOMA") + ": " + LANG_NAMES[lang], 14, GREEN, true);
        langBtn.setLetterSpacing(0.1f);
        langBtn.setGravity(Gravity.CENTER);
        langBtn.setBackground(ripple(box(PANEL, GREEN_DEEP, 2, 14)));

        langBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showLanguageDialog();
            }
        });

        LinearLayout.LayoutParams langLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                dp(48)
        );

        langLp.topMargin = dp(12);
        col.addView(langBtn, langLp);

        TextView credit = text("Port by Kali Kyle", 14, RED, false);
        credit.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams crLp = wrap();
        crLp.topMargin = dp(30);
        col.addView(credit, crLp);

        TextView hint = text(tr("Requer o app Xash3D FWGS instalado", "Requires the Xash3D FWGS app installed", "Requiere la app Xash3D FWGS instalada"), 12, GREEN, false);
        hint.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams hintLp = wrap();
        hintLp.topMargin = dp(4);
        col.addView(hint, hintLp);

        return root;
    }

    private View linkButton(Drawable icon, String label, final String url) {
        LinearLayout b = new LinearLayout(this);
        b.setOrientation(LinearLayout.VERTICAL);
        b.setGravity(Gravity.CENTER);
        b.setPadding(dp(4), dp(12), dp(4), dp(10));
        b.setBackground(ripple(box(PANEL, GREEN_DEEP, 2, 14)));

        ImageView iv = new ImageView(this);
        iv.setImageDrawable(icon);

        b.addView(iv, new LinearLayout.LayoutParams(dp(30), dp(30)));

        TextView t = text(label, 12, GREEN, true);
        t.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams tLp = wrap();
        tLp.topMargin = dp(6);
        b.addView(t, tLp);

        b.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openUrl(url);
            }
        });

        return b;
    }

    private LinearLayout.LayoutParams cell(boolean leftMargin) {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                0,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                1f
        );

        if (leftMargin) {
            lp.leftMargin = dp(10);
        }

        return lp;
    }

    // SECAO DE DOWNLOADS

    private void setupDownloadsButton() {
        android.app.ActionBar bar = getActionBar();

        if (bar == null) {
            return;
        }

        LinearLayout btn = new LinearLayout(this);
        btn.setOrientation(LinearLayout.HORIZONTAL);
        btn.setGravity(Gravity.CENTER_VERTICAL);
        btn.setPadding(dp(12), dp(6), dp(12), dp(6));
        btn.setBackground(ripple(box(0x00000000, GREEN_DEEP, 1, 18)));

        ImageView icon = new ImageView(this);
        icon.setImageDrawable(new PolyIcon(DOWNLOAD, GREEN, dp(20)));
        btn.addView(icon, new LinearLayout.LayoutParams(dp(20), dp(20)));

        TextView label = text(tr("Downloads", "Downloads", "Descargas"), 14, GREEN, true);

        LinearLayout.LayoutParams labelLp = wrap();
        labelLp.leftMargin = dp(6);
        btn.addView(label, labelLp);

        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showDownloadsDialog();
            }
        });

        android.app.ActionBar.LayoutParams lp = new android.app.ActionBar.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                Gravity.END | Gravity.CENTER_VERTICAL
        );

        lp.rightMargin = dp(8);

        bar.setCustomView(btn, lp);
        bar.setDisplayShowCustomEnabled(true);
    }

    private void showDownloadsDialog() {
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(dp(20), dp(20), dp(20), dp(16));

        LinearLayout head = new LinearLayout(this);
        head.setOrientation(LinearLayout.HORIZONTAL);
        head.setGravity(Gravity.CENTER_VERTICAL);

        ImageView icon = new ImageView(this);
        icon.setImageDrawable(new PolyIcon(DOWNLOAD, RED, dp(22)));
        head.addView(icon, new LinearLayout.LayoutParams(dp(22), dp(22)));

        TextView title = text(tr("DOWNLOADS", "DOWNLOADS", "DESCARGAS"), 18, RED, true);
        title.setLetterSpacing(0.15f);

        LinearLayout.LayoutParams titleLp = wrap();
        titleLp.leftMargin = dp(10);
        head.addView(title, titleLp);

        box.addView(head, wrap());

        TextView hint = text(tr("Os arquivos s\u00e3o extra\u00eddos em ", "Files are extracted to ", "Los archivos se extraen en ") + XASH_DIR, 12, GREEN, false);

        LinearLayout.LayoutParams hintLp = wrap();
        hintLp.topMargin = dp(8);
        box.addView(hint, hintLp);

        for (DownloadItem d : downloads) {
            d.refreshInstalled();

            LinearLayout.LayoutParams cardLp = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
            );

            cardLp.topMargin = dp(12);
            box.addView(d.createView(), cardLp);
            d.render();
        }

        ScrollView sv = new ScrollView(this);

        sv.addView(box, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        ));

        if (downloadsDialog != null) {
            downloadsDialog.dismiss();
        }

        downloadsDialog = new AlertDialog.Builder(this)
                .setView(sv)
                .setPositiveButton(tr("Fechar", "Close", "Cerrar"), null)
                .create();

        downloadsDialog.getWindow().setBackgroundDrawable(box(BG, GREEN_DEEP, 2, 18));
        downloadsDialog.show();
        downloadsDialog.getButton(AlertDialog.BUTTON_POSITIVE).setTextColor(GREEN);
    }

    private static String fmtBytes(long b) {
        if (b < 1024L * 1024L) {
            return Math.max(1, b / 1024L) + " KB";
        }

        double mb = b / (1024.0 * 1024.0);

        if (mb < 1024.0) {
            return String.format(java.util.Locale.US, "%.1f MB", mb);
        }

        return String.format(java.util.Locale.US, "%.2f GB", mb / 1024.0);
    }

    private static long freeSpace(File dir) {
        try {
            StatFs fs = new StatFs(dir.getPath());

            if (Build.VERSION.SDK_INT >= 18) {
                return fs.getAvailableBytes();
            }

            return (long) fs.getAvailableBlocks() * (long) fs.getBlockSize();
        } catch (Exception e) {
            return Long.MAX_VALUE;
        }
    }

    private static class CancelException extends IOException {
        CancelException() {
            super("cancelado");
        }
    }

    private class DownloadItem {
        final String id;
        final String title;
        final String subtitle;
        final String url;
        final String dirName;
        final String zipName;

        volatile int state = ST_IDLE;
        volatile boolean cancelled = false;
        volatile boolean installed = false;
        volatile int progress = 0; // 0..1000
        volatile boolean indeterminate = false;
        volatile String detail = "";
        volatile String error = null;

        TextView statusView;
        TextView buttonView;
        ProgressBar bar;

        final Runnable renderRunnable = new Runnable() {
            @Override
            public void run() {
                render();
            }
        };

        String subtitleText() {
            if ("valve".equals(id)) {
                return tr("Arquivos obrigat\u00f3rios para o funcionamento do jogo", "Required files for the game to run", "Archivos obligatorios para el funcionamiento del juego");
            }

            return tr("Arquivos necess\u00e1rios para o mod", "Files required for the mod", "Archivos necesarios para el mod");
        }

        DownloadItem(String id, String title, String subtitle, String url, String dirName, String zipName) {
            this.id = id;
            this.title = title;
            this.subtitle = subtitle;
            this.url = url;
            this.dirName = dirName;
            this.zipName = zipName;
        }

        View createView() {
            LinearLayout card = new LinearLayout(MainActivity.this);
            card.setOrientation(LinearLayout.VERTICAL);
            card.setPadding(dp(14), dp(12), dp(14), dp(14));
            card.setBackground(box(PANEL, GREEN_DEEP, 2, 14));

            LinearLayout top = new LinearLayout(MainActivity.this);
            top.setOrientation(LinearLayout.HORIZONTAL);
            top.setGravity(Gravity.CENTER_VERTICAL);

            ImageView iv = new ImageView(MainActivity.this);
            iv.setImageDrawable(new PolyIcon(DOWNLOAD, RED, dp(24)));
            top.addView(iv, new LinearLayout.LayoutParams(dp(24), dp(24)));

            TextView t = text(title, 14, RED, true);

            LinearLayout.LayoutParams tLp = new LinearLayout.LayoutParams(
                    0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
            tLp.leftMargin = dp(10);
            top.addView(t, tLp);

            card.addView(top, new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));

            TextView sub = text(subtitleText(), 12, GREEN, false);

            LinearLayout.LayoutParams subLp = wrap();
            subLp.topMargin = dp(6);
            card.addView(sub, subLp);

            statusView = text("", 12, 0xFFE8E8E8, false);

            LinearLayout.LayoutParams stLp = wrap();
            stLp.topMargin = dp(8);
            card.addView(statusView, stLp);

            bar = new ProgressBar(MainActivity.this, null, android.R.attr.progressBarStyleHorizontal);
            bar.setMax(1000);

            GradientDrawable track = new GradientDrawable();
            track.setColor(BG);
            track.setStroke(dp(1), GREEN_DEEP);
            track.setCornerRadius(dp(4));

            GradientDrawable fill = new GradientDrawable();
            fill.setColor(GREEN);
            fill.setCornerRadius(dp(4));

            LayerDrawable layers = new LayerDrawable(new Drawable[] {
                    track,
                    new ClipDrawable(fill, Gravity.LEFT, ClipDrawable.HORIZONTAL)
            });
            layers.setId(0, android.R.id.background);
            layers.setId(1, android.R.id.progress);
            bar.setProgressDrawable(layers);

            LinearLayout.LayoutParams barLp = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, dp(8));
            barLp.topMargin = dp(8);
            card.addView(bar, barLp);

            buttonView = text("", 13, RED, true);
            buttonView.setLetterSpacing(0.1f);
            buttonView.setGravity(Gravity.CENTER);

            buttonView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    onButton();
                }
            });

            LinearLayout.LayoutParams bLp = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, dp(46));
            bLp.topMargin = dp(12);
            card.addView(buttonView, bLp);

            return card;
        }

        void onButton() {
            if (state != ST_IDLE) {
                cancelled = true;
                detail = tr("Cancelando...", "Cancelling...", "Cancelando...");
                render();
                return;
            }

            if (installed) {
                return;
            }

            if (!hasStoragePermission()) {
                requestStoragePermission(this);
                return;
            }

            start();
        }

        void render() {
            if (buttonView == null) {
                return;
            }

            boolean running = state != ST_IDLE;

            bar.setVisibility(running ? View.VISIBLE : View.GONE);

            if (running) {
                bar.setIndeterminate(indeterminate);

                if (!indeterminate) {
                    bar.setProgress(progress);
                }

                statusView.setTextColor(0xFFE8E8E8);
                statusView.setText(detail);

                buttonView.setText(tr("CANCELAR", "CANCEL", "CANCELAR"));
                buttonView.setTextColor(RED);
                buttonView.setEnabled(true);
                buttonView.setBackground(ripple(box(PANEL, RED, 2, 12)));
            } else if (installed) {
                statusView.setTextColor(GREEN);
                statusView.setText(tr("Instalado em ", "Installed in ", "Instalado en ") + XASH_DIR + "/" + dirName);

                buttonView.setText(tr("Arquivos j\u00e1 instalados", "Files already installed", "Archivos ya instalados"));
                buttonView.setTextColor(GREEN);
                buttonView.setEnabled(false);
                buttonView.setBackground(box(PANEL, GREEN_DEEP, 2, 12));
            } else {
                if (error != null) {
                    statusView.setTextColor(RED);
                    statusView.setText(error);
                } else {
                    statusView.setTextColor(0xFFE8E8E8);
                    statusView.setText(tr("Arquivos n\u00e3o instalados", "Files not installed", "Archivos no instalados"));
                }

                buttonView.setText(tr("Baixar arquivos", "Download Files", "Descargar archivos"));
                buttonView.setTextColor(RED);
                buttonView.setEnabled(true);
                buttonView.setBackground(ripple(box(RED_DARK, RED, 2, 12)));
            }
        }

        // ---------- verificacao de instalacao ----------

        void refreshInstalled() {
            if (state != ST_IDLE) {
                return;
            }

            installed = checkInstalled();
        }

        private boolean checkInstalled() {
            File base = new File(XASH_DIR);

            String roots = getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                    .getString("dl_roots_" + id, null);

            if (roots != null && roots.length() > 0) {
                boolean all = true;

                for (String r : roots.split("\\|")) {
                    if (r.length() > 0 && !new File(base, r).exists()) {
                        all = false;
                    }
                }

                if (all) {
                    return true;
                }
            }

            // instalacao manual (sem registro do launcher): basta a pasta existir
            return dirCheck(base);
        }

        private File findDir(File base) {
            File exact = new File(base, dirName);

            if (exact.isDirectory()) {
                return exact;
            }

            File[] list = base.listFiles();

            if (list != null) {
                for (File f : list) {
                    if (f.isDirectory() && f.getName().equalsIgnoreCase(dirName)) {
                        return f;
                    }
                }
            }

            return null;
        }

        private boolean dirCheck(File base) {
            File d = findDir(base);

            if (d == null) {
                return false;
            }

            String[] names = d.list();

            // names == null: pasta existe mas nao da para listar (sem permissao) -> considera instalada
            return names == null || names.length > 0;
        }

        private boolean hasAnyFile(File dir, int depth) {
            if (depth < 0 || !dir.isDirectory()) {
                return false;
            }

            File[] files = dir.listFiles();

            if (files == null) {
                return false;
            }

            for (File f : files) {
                if (f.isFile()) {
                    return true;
                }
            }

            for (File f : files) {
                if (f.isDirectory() && hasAnyFile(f, depth - 1)) {
                    return true;
                }
            }

            return false;
        }

        // ---------- execucao ----------

        void start() {
            if (state != ST_IDLE) {
                return;
            }

            cancelled = false;
            error = null;
            progress = 0;
            indeterminate = true;
            detail = tr("Conectando...", "Connecting...", "Conectando...");
            state = ST_DOWNLOADING;

            updateKeepScreenOn();
            render();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    work();
                }
            }, "download-" + id).start();
        }

        private void post(int st, int prog, boolean indet, String text) {
            state = st;
            progress = prog;
            indeterminate = indet;
            detail = text;
            ui.post(renderRunnable);
        }

        private void work() {
            File tmp = null;
            String result = null;
            final List<File> created = new ArrayList<File>();
            boolean ok = false;
            LinkedHashSet<String> roots = null;

            try {
                File base = new File(XASH_DIR);

                if (!base.isDirectory() && !base.mkdirs()) {
                    throw new IOException(tr("N\u00e3o foi poss\u00edvel criar a pasta ", "Could not create folder ", "No se pudo crear la carpeta ") + XASH_DIR
                            + tr(". Verifique a permiss\u00e3o de armazenamento.", ". Check the storage permission.", ". Verifique el permiso de almacenamiento."));
                }

                tmp = new File(base, "." + zipName + ".part");

                download(base, tmp);
                roots = install(base, tmp, created);
                ok = true;
            } catch (CancelException e) {
                result = tr("Cancelado.", "Cancelled.", "Cancelado.");
            } catch (Exception e) {
                result = friendlyError(e);
            } finally {
                if (tmp != null && tmp.exists()) {
                    tmp.delete();
                }
            }

            if (!ok) {
                // desfaz arquivos novos criados nesta tentativa, para nao parecer instalado pela metade
                for (int i = created.size() - 1; i >= 0; i--) {
                    created.get(i).delete();
                }
            } else if (roots != null && roots.size() <= 50) {
                StringBuilder sb = new StringBuilder();

                for (String r : roots) {
                    if (sb.length() > 0) {
                        sb.append('|');
                    }

                    sb.append(r);
                }

                getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                        .edit()
                        .putString("dl_roots_" + id, sb.toString())
                        .apply();
            }

            final boolean success = ok;
            final String msg = result;

            ui.post(new Runnable() {
                @Override
                public void run() {
                    state = ST_IDLE;
                    cancelled = false;
                    indeterminate = false;
                    error = success ? null : msg;
                    installed = success || checkInstalledAfterFail();

                    updateKeepScreenOn();
                    render();

                    if (success) {
                        Toast.makeText(MainActivity.this,
                                title + tr(" instalado com sucesso", " installed successfully", " instalado con \u00e9xito"),
                                Toast.LENGTH_LONG).show();
                    }
                }
            });
        }

        private boolean checkInstalledAfterFail() {
            // falha/cancelamento nao deve marcar como instalado, a menos que ja estivesse antes
            return installed && checkInstalled();
        }

        private HttpURLConnection open() throws IOException {
            String u = url.replace("dl=0", "dl=1");

            for (int i = 0; i < 10; i++) {
                if (cancelled) {
                    throw new CancelException();
                }

                HttpURLConnection c = (HttpURLConnection) new URL(u).openConnection();
                c.setInstanceFollowRedirects(false);
                c.setConnectTimeout(20000);
                c.setReadTimeout(30000);
                c.setRequestProperty("User-Agent", "Mozilla/5.0 (Linux; Android) HalfForceLauncher");

                int code = c.getResponseCode();

                if (code == 301 || code == 302 || code == 303 || code == 307 || code == 308) {
                    String loc = c.getHeaderField("Location");
                    c.disconnect();

                    if (loc == null) {
                        throw new IOException(tr("Redirecionamento inv\u00e1lido do servidor", "Invalid redirect from the server", "Redirecci\u00f3n no v\u00e1lida del servidor"));
                    }

                    u = new URL(new URL(u), loc).toString();
                    continue;
                }

                if (code == 200) {
                    return c;
                }

                c.disconnect();

                if (code == 404 || code == 403 || code == 410) {
                    throw new IOException(tr("Link do Dropbox indispon\u00edvel (HTTP ", "Dropbox link unavailable (HTTP ", "Enlace de Dropbox no disponible (HTTP ") + code + ")");
                }

                throw new IOException(tr("O servidor respondeu com erro HTTP ", "The server responded with HTTP error ", "El servidor respondi\u00f3 con error HTTP ") + code);
            }

            throw new IOException(tr("Redirecionamentos demais ao acessar o link", "Too many redirects while accessing the link", "Demasiadas redirecciones al acceder al enlace"));
        }

        private void download(File base, File tmp) throws IOException {
            HttpURLConnection c = open();

            try {
                String type = c.getContentType();

                if (type != null && type.toLowerCase().startsWith("text/html")) {
                    throw new IOException(tr("O link n\u00e3o retornou um arquivo ZIP (link inv\u00e1lido ou expirado)", "The link did not return a ZIP file (invalid or expired link)", "El enlace no devolvi\u00f3 un archivo ZIP (enlace no v\u00e1lido o caducado)"));
                }

                long total = -1;

                try {
                    String len = c.getHeaderField("Content-Length");

                    if (len != null) {
                        total = Long.parseLong(len.trim());
                    }
                } catch (NumberFormatException ignored) {
                }

                if (total > 0 && freeSpace(base) < total) {
                    throw new IOException(tr("Espa\u00e7o insuficiente: s\u00e3o necess\u00e1rios pelo menos ", "Not enough space: at least ", "Espacio insuficiente: se necesitan al menos ")
                            + fmtBytes(total) + tr(" livres para baixar", " of free space is needed to download", " libres para descargar"));
                }

                InputStream in = c.getInputStream();
                OutputStream out = new FileOutputStream(tmp);

                try {
                    byte[] buf = new byte[65536];
                    long done = 0;
                    long last = 0;
                    int n;

                    while ((n = in.read(buf)) != -1) {
                        if (cancelled) {
                            throw new CancelException();
                        }

                        out.write(buf, 0, n);
                        done += n;

                        long now = SystemClock.uptimeMillis();

                        if (now - last >= 250) {
                            last = now;

                            if (total > 0) {
                                int p = (int) (done * 1000L / total);
                                post(ST_DOWNLOADING, p, false,
                                        tr("Baixando... ", "Downloading... ", "Descargando... ") + (p / 10) + "%  (" + fmtBytes(done) + " / " + fmtBytes(total) + ")");
                            } else {
                                post(ST_DOWNLOADING, 0, true, tr("Baixando... ", "Downloading... ", "Descargando... ") + fmtBytes(done));
                            }
                        }
                    }

                    out.flush();

                    if (total > 0 && done != total) {
                        throw new IOException(tr("Download incompleto (", "Incomplete download (", "Descarga incompleta (") + fmtBytes(done) + tr(" de ", " of ", " de ") + fmtBytes(total)
                                + tr("). Verifique a conex\u00e3o e tente novamente", "). Check your connection and try again.", "). Verifique su conexi\u00f3n e int\u00e9ntelo de nuevo."));
                    }
                } finally {
                    try {
                        out.close();
                    } catch (IOException ignored) {
                    }

                    try {
                        in.close();
                    } catch (IOException ignored) {
                    }
                }
            } finally {
                c.disconnect();
            }
        }

        private LinkedHashSet<String> install(File base, File zip, List<File> created) throws IOException {
            post(ST_INSTALLING, 0, true, tr("Preparando instala\u00e7\u00e3o...", "Preparing installation...", "Preparando instalaci\u00f3n..."));

            ZipFile zf = new ZipFile(zip);

            try {
                long totalBytes = 0;
                int count = 0;

                Enumeration<? extends ZipEntry> en = zf.entries();

                while (en.hasMoreElements()) {
                    ZipEntry e = en.nextElement();
                    count++;

                    if (!e.isDirectory() && e.getSize() > 0) {
                        totalBytes += e.getSize();
                    }
                }

                if (count == 0) {
                    throw new ZipException("ZIP vazio");
                }

                if (freeSpace(base) < totalBytes) {
                    throw new IOException(tr("Espa\u00e7o insuficiente: s\u00e3o necess\u00e1rios mais ", "Not enough space: you need ", "Espacio insuficiente: se necesitan ")
                            + fmtBytes(totalBytes) + tr(" livres para instalar", " more free space to install", " m\u00e1s de espacio libre para instalar"));
                }

                String baseCanon = base.getCanonicalPath();
                LinkedHashSet<String> roots = new LinkedHashSet<String>();

                byte[] buf = new byte[65536];
                long done = 0;
                int idx = 0;
                long last = 0;

                en = zf.entries();

                while (en.hasMoreElements()) {
                    if (cancelled) {
                        throw new CancelException();
                    }

                    ZipEntry e = en.nextElement();
                    idx++;

                    String name = e.getName().replace('\\', '/');

                    while (name.startsWith("/")) {
                        name = name.substring(1);
                    }

                    if (name.length() == 0 || name.startsWith("__MACOSX/")) {
                        continue;
                    }

                    File out = new File(base, name);
                    String canon = out.getCanonicalPath();

                    if (!canon.equals(baseCanon) && !canon.startsWith(baseCanon + File.separator)) {
                        throw new IOException(tr("Entrada inv\u00e1lida no ZIP: ", "Invalid entry in ZIP: ", "Entrada no v\u00e1lida en el ZIP: ") + name);
                    }

                    int slash = name.indexOf('/');
                    roots.add(slash < 0 ? name : name.substring(0, slash));

                    if (e.isDirectory()) {
                        if (!out.isDirectory() && !out.mkdirs()) {
                            throw new IOException(tr("N\u00e3o foi poss\u00edvel criar a pasta ", "Could not create folder ", "No se pudo crear la carpeta ") + name);
                        }

                        continue;
                    }

                    File parent = out.getParentFile();

                    if (parent != null && !parent.isDirectory() && !parent.mkdirs()) {
                        throw new IOException(tr("N\u00e3o foi poss\u00edvel criar a pasta ", "Could not create folder ", "No se pudo crear la carpeta ") + parent.getPath());
                    }

                    boolean existed = out.exists();

                    if (!existed) {
                        created.add(out);
                    }

                    InputStream in = zf.getInputStream(e);
                    OutputStream os = new BufferedOutputStream(new FileOutputStream(out), 65536);

                    try {
                        int n;

                        while ((n = in.read(buf)) != -1) {
                            if (cancelled) {
                                throw new CancelException();
                            }

                            os.write(buf, 0, n);
                            done += n;

                            long now = SystemClock.uptimeMillis();

                            if (now - last >= 250) {
                                last = now;
                                postInstall(done, totalBytes, idx, count);
                            }
                        }
                    } finally {
                        try {
                            os.close();
                        } catch (IOException ignored) {
                        }

                        try {
                            in.close();
                        } catch (IOException ignored) {
                        }
                    }
                }

                post(ST_INSTALLING, 1000, false, tr("Instalando... 100%", "Installing... 100%", "Instalando... 100%"));
                return roots;
            } finally {
                try {
                    zf.close();
                } catch (IOException ignored) {
                }
            }
        }

        private void postInstall(long done, long totalBytes, int idx, int count) {
            int p;

            if (totalBytes > 0) {
                p = (int) (done * 1000L / totalBytes);
            } else {
                p = (int) (idx * 1000L / count);
            }

            if (p > 1000) {
                p = 1000;
            }

            post(ST_INSTALLING, p, false,
                    tr("Instalando... ", "Installing... ", "Instalando... ") + (p / 10) + "%  (" + tr("arquivo ", "file ", "archivo ") + idx + tr(" de ", " of ", " de ") + count + ")");
        }

        private String friendlyError(Exception e) {
            if (e instanceof UnknownHostException
                    || e instanceof SocketTimeoutException
                    || e instanceof SocketException) {
                return tr("Falha de conex\u00e3o com a internet. Verifique a rede e tente novamente.", "Internet connection failed. Check your network and try again.", "Fallo de conexi\u00f3n a internet. Verifique la red e int\u00e9ntelo de nuevo.");
            }

            if (e instanceof ZipException) {
                return tr("O arquivo baixado est\u00e1 corrompido (ZIP inv\u00e1lido). Tente baixar novamente.", "The downloaded file is corrupted (invalid ZIP). Try downloading again.", "El archivo descargado est\u00e1 da\u00f1ado (ZIP no v\u00e1lido). Intente descargar de nuevo.");
            }

            if (e instanceof SecurityException || e instanceof FileNotFoundException) {
                String m = e.getMessage() == null ? "" : e.getMessage();

                if (m.contains("ENOSPC")) {
                    return tr("Sem espa\u00e7o no armazenamento.", "Out of storage space.", "Sin espacio de almacenamiento.");
                }

                return tr("Sem permiss\u00e3o para gravar em ", "No permission to write to ", "Sin permiso para escribir en ") + XASH_DIR + tr(". Conceda a permiss\u00e3o de armazenamento.", ". Grant the storage permission.", ". Conceda el permiso de almacenamiento.");
            }

            String m = e.getMessage();

            if (m != null && m.contains("ENOSPC")) {
                return tr("Sem espa\u00e7o no armazenamento.", "Out of storage space.", "Sin espacio de almacenamiento.");
            }

            return tr("Erro: ", "Error: ", "Error: ") + (m == null ? e.getClass().getSimpleName() : m);
        }
    }

    // JANELA APOIE-ME

    private void showSupportDialog() {
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(dp(20), dp(20), dp(20), dp(16));

        TextView title = text(tr("APOIE O PROJETO", "SUPPORT THE PROJECT", "APOYA EL PROYECTO"), 18, RED, true);
        title.setLetterSpacing(0.15f);
        box.addView(title, wrap());

        TextView note = text(
                tr("Envie somente USDT pela rede indicada e confira o endere\u00e7o antes de enviar. Obrigado por apoiar!", "Send only USDT on the indicated network and double-check the address before sending. Thank you for your support!", "Env\u00ede solo USDT por la red indicada y verifique la direcci\u00f3n antes de enviar. \u00a1Gracias por apoyar!"),
                13, GREEN, false
        );

        LinearLayout.LayoutParams noteLp = wrap();
        noteLp.topMargin = dp(8);
        noteLp.bottomMargin = dp(8);
        box.addView(note, noteLp);

        for (int i = 0; i < WALLETS.length; i++) {
            final String label = WALLETS[i][0];
            final String address = WALLETS[i][1];

            LinearLayout card = new LinearLayout(this);
            card.setOrientation(LinearLayout.VERTICAL);
            card.setPadding(dp(14), dp(12), dp(14), dp(12));
            card.setBackground(box(PANEL, GREEN_DEEP, 2, 12));

            card.addView(text(label, 14, RED, true), wrap());

            TextView addr = text(address, 12, GREEN, false);
            addr.setTypeface(Typeface.MONOSPACE);
            addr.setTextIsSelectable(true);

            LinearLayout.LayoutParams addrLp = wrap();
            addrLp.topMargin = dp(6);
            card.addView(addr, addrLp);

            TextView copy = text(tr("COPIAR", "COPY", "COPIAR"), 13, RED, true);
            copy.setLetterSpacing(0.2f);
            copy.setGravity(Gravity.CENTER);
            copy.setBackground(ripple(box(0x00000000, RED, 2, 10)));

            copy.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    copyToClipboard(label, address);
                }
            });

            LinearLayout.LayoutParams copyLp = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    dp(40)
            );

            copyLp.topMargin = dp(10);
            card.addView(copy, copyLp);

            LinearLayout.LayoutParams cardLp = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
            );

            cardLp.topMargin = dp(8);
            box.addView(card, cardLp);
        }

        ScrollView sv = new ScrollView(this);

        sv.addView(box, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        ));

        AlertDialog dialog = new AlertDialog.Builder(this)
                .setView(sv)
                .setPositiveButton(tr("Fechar", "Close", "Cerrar"), null)
                .create();

        dialog.getWindow().setBackgroundDrawable(box(BG, GREEN_DEEP, 2, 18));
        dialog.show();
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setTextColor(GREEN);
    }

    private void copyToClipboard(String label, String address) {
        ClipboardManager cm = (ClipboardManager)
                getSystemService(Context.CLIPBOARD_SERVICE);

        if (cm != null) {
            cm.setPrimaryClip(ClipData.newPlainText(label, address));
            Toast.makeText(this, tr("Endere\u00e7o copiado", "Address copied", "Direcci\u00f3n copiada"), Toast.LENGTH_SHORT).show();
        }
    }

    // UTILIDADES

    private int dp(int v) {
        return Math.round(TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                v,
                getResources().getDisplayMetrics()
        ));
    }

    private LinearLayout.LayoutParams wrap() {
        return new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        );
    }

    private TextView text(String s, int sp, int color, boolean bold) {
        TextView t = new TextView(this);
        t.setText(s);
        t.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        t.setTextColor(color);
        t.setTypeface(bold ? Typeface.DEFAULT_BOLD : Typeface.DEFAULT);
        return t;
    }

    private GradientDrawable box(int fill, int stroke, int strokeDp, int radiusDp) {
        GradientDrawable g = new GradientDrawable();
        g.setShape(GradientDrawable.RECTANGLE);
        g.setColor(fill);
        g.setStroke(dp(strokeDp), stroke);
        g.setCornerRadius(dp(radiusDp));
        return g;
    }

    private Drawable ripple(Drawable content) {
        return new RippleDrawable(
                ColorStateList.valueOf(0x33FFFFFF),
                content,
                null
        );
    }

    // ICONES

    private static final float[][] DISCORD = {
        {20.32f, 4.37f, 19.73f, 4.11f, 19.14f, 3.87f, 18.53f, 3.65f, 17.92f, 3.45f, 17.31f, 3.27f, 16.69f, 3.11f, 16.06f, 2.97f, 15.43f, 2.85f, 15.41f, 2.85f, 15.39f, 2.86f, 15.36f, 2.88f, 15.22f, 3.13f, 15.10f, 3.38f, 14.97f, 3.64f, 14.85f, 3.90f, 14.75f, 4.14f, 13.64f, 4.01f, 12.54f, 3.94f, 11.44f, 3.94f, 10.35f, 4.01f, 9.26f, 4.14f, 9.15f, 3.90f, 9.03f, 3.64f, 8.90f, 3.38f, 8.77f, 3.13f, 8.64f, 2.89f, 8.62f, 2.87f, 8.60f, 2.86f, 8.57f, 2.85f, 7.93f, 2.97f, 7.31f, 3.11f, 6.69f, 3.27f, 6.07f, 3.45f, 5.46f, 3.65f, 4.86f, 3.87f, 4.26f, 4.11f, 3.68f, 4.37f, 3.65f, 4.38f, 2.04f, 7.17f, 0.93f, 9.92f, 0.27f, 12.65f, 0.01f, 15.36f, 0.10f, 18.06f, 0.11f, 18.08f, 0.12f, 18.10f, 1.35f, 18.95f, 2.56f, 19.66f, 3.76f, 20.25f, 4.95f, 20.74f, 6.12f, 21.14f, 6.15f, 21.15f, 6.17f, 21.14f, 6.19f, 21.13f, 6.48f, 20.73f, 6.74f, 20.34f, 6.98f, 19.94f, 7.21f, 19.54f, 7.43f, 19.12f, 7.44f, 19.09f, 7.44f, 19.06f, 7.42f, 19.03f, 7.39f, 19.02f, 7.00f, 18.86f, 6.62f, 18.69f, 6.25f, 18.51f, 5.88f, 18.32f, 5.52f, 18.12f, 5.49f, 18.10f, 5.48f, 18.06f, 5.49f, 18.02f, 5.51f, 18.00f, 5.59f, 17.94f, 5.66f, 17.88f, 5.74f, 17.82f, 5.81f, 17.76f, 5.88f, 17.70f, 5.91f, 17.69f, 5.93f, 17.69f, 5.95f, 17.69f, 8.35f, 18.55f, 10.79f, 18.99f, 13.23f, 18.99f, 15.66f, 18.55f, 18.02f, 17.69f, 18.05f, 17.69f, 18.07f, 17.69f, 18.10f, 17.70f, 18.17f, 17.76f, 18.25f, 17.82f, 18.32f, 17.88f, 18.40f, 17.94f, 18.47f, 18.00f, 18.50f, 18.02f, 18.50f, 18.06f, 18.49f, 18.10f, 18.47f, 18.12f, 18.24f, 18.25f, 18.01f, 18.38f, 17.78f, 18.49f, 17.55f, 18.61f, 17.31f, 18.72f, 17.08f, 18.82f, 16.84f, 18.92f, 16.59f, 19.01f, 16.57f, 19.03f, 16.55f, 19.06f, 16.55f, 19.09f, 16.55f, 19.12f, 16.78f, 19.54f, 17.01f, 19.94f, 17.26f, 20.34f, 17.51f, 20.73f, 17.78f, 21.11f, 17.81f, 21.14f, 17.83f, 21.15f, 17.85f, 21.15f, 19.04f, 20.74f, 20.23f, 20.25f, 21.43f, 19.66f, 22.64f, 18.95f, 23.87f, 18.11f, 23.89f, 18.09f, 23.90f, 18.07f, 23.98f, 15.03f, 23.64f, 12.16f, 22.91f, 9.44f, 21.81f, 6.85f, 20.35f, 4.40f, 20.33f, 4.38f},
        {8.02f, 15.33f, 7.34f, 15.21f, 6.75f, 14.86f, 6.28f, 14.34f, 5.97f, 13.68f, 5.86f, 12.91f, 5.97f, 12.15f, 6.28f, 11.48f, 6.74f, 10.96f, 7.33f, 10.62f, 8.02f, 10.49f, 8.71f, 10.62f, 9.31f, 10.96f, 9.77f, 11.49f, 10.08f, 12.15f, 10.18f, 12.91f, 10.07f, 13.68f, 9.76f, 14.34f, 9.30f, 14.86f, 8.71f, 15.21f, 8.02f, 15.33f},
        {15.99f, 15.33f, 15.32f, 15.21f, 14.72f, 14.86f, 14.26f, 14.34f, 13.95f, 13.68f, 13.84f, 12.91f, 13.95f, 12.15f, 14.25f, 11.48f, 14.72f, 10.96f, 15.31f, 10.62f, 15.99f, 10.49f, 16.69f, 10.62f, 17.28f, 10.96f, 17.75f, 11.49f, 18.05f, 12.15f, 18.15f, 12.91f, 18.04f, 13.68f, 17.74f, 14.34f, 17.28f, 14.86f, 16.68f, 15.21f, 15.99f, 15.33f}
    };

    private static final float[][] DOWNLOAD = {
        {5f, 20f, 19f, 20f, 19f, 18f, 5f, 18f},
        {19f, 9f, 15f, 9f, 15f, 3f, 9f, 3f, 9f, 9f, 5f, 9f, 12f, 16f}
    };

    private static final float[][] GITHUB = {
        {12.00f, 0.30f, 8.21f, 0.91f, 4.91f, 2.61f, 2.31f, 5.21f, 0.61f, 8.50f, 0.00f, 12.30f, 0.40f, 15.37f, 1.52f, 18.14f, 3.26f, 20.52f, 5.52f, 22.40f, 8.21f, 23.68f, 8.52f, 23.70f, 8.75f, 23.62f, 8.91f, 23.48f, 9.00f, 23.30f, 9.03f, 23.11f, 9.02f, 22.88f, 9.02f, 22.55f, 9.02f, 22.13f, 9.01f, 21.63f, 9.01f, 21.07f, 7.31f, 21.18f, 6.15f, 20.81f, 5.43f, 20.23f, 5.07f, 19.69f, 4.97f, 19.46f, 4.62f, 18.74f, 4.26f, 18.24f, 3.95f, 17.92f, 3.72f, 17.75f, 3.63f, 17.70f, 3.22f, 17.34f, 3.19f, 17.12f, 3.37f, 17.01f, 3.60f, 16.98f, 3.72f, 16.97f, 4.37f, 17.13f, 4.88f, 17.44f, 5.26f, 17.80f, 5.48f, 18.09f, 5.55f, 18.21f, 6.26f, 19.04f, 7.05f, 19.44f, 7.83f, 19.51f, 8.53f, 19.40f, 9.05f, 19.21f, 9.14f, 18.77f, 9.27f, 18.39f, 9.43f, 18.07f, 9.61f, 17.81f, 9.81f, 17.60f, 8.22f, 17.31f, 6.73f, 16.71f, 5.50f, 15.66f, 4.66f, 14.02f, 4.34f, 11.67f, 4.40f, 10.91f, 4.56f, 10.21f, 4.81f, 9.57f, 5.15f, 8.98f, 5.58f, 8.45f, 5.48f, 8.16f, 5.37f, 7.68f, 5.33f, 7.02f, 5.41f, 6.21f, 5.68f, 5.27f, 5.81f, 5.25f, 6.18f, 5.26f, 6.83f, 5.40f, 7.76f, 5.78f, 8.98f, 6.50f, 9.57f, 6.36f, 10.16f, 6.25f, 10.76f, 6.17f, 11.37f, 6.12f, 11.98f, 6.10f, 12.60f, 6.12f, 13.20f, 6.17f, 13.81f, 6.25f, 14.40f, 6.36f, 14.98f, 6.50f, 16.20f, 5.78f, 17.13f, 5.40f, 17.77f, 5.26f, 18.15f, 5.25f, 18.27f, 5.27f, 18.54f, 6.21f, 18.62f, 7.02f, 18.58f, 7.68f, 18.48f, 8.16f, 18.39f, 8.45f, 18.81f, 8.98f, 19.15f, 9.57f, 19.41f, 10.21f, 19.56f, 10.91f, 19.62f, 11.67f, 19.31f, 14.03f, 18.46f, 15.66f, 17.22f, 16.71f, 15.74f, 17.30f, 14.14f, 17.59f, 14.39f, 17.85f, 14.61f, 18.20f, 14.79f, 18.65f, 14.91f, 19.18f, 14.95f, 19.81f, 14.95f, 20.73f, 14.95f, 21.55f, 14.94f, 22.23f, 14.94f, 22.76f, 14.94f, 23.10f, 14.97f, 23.29f, 15.05f, 23.47f, 15.21f, 23.61f, 15.44f, 23.68f, 15.76f, 23.67f, 18.46f, 22.39f, 20.74f, 20.51f, 22.48f, 18.13f, 23.60f, 15.36f, 24.00f, 12.30f, 23.39f, 8.50f, 21.68f, 5.21f, 19.09f, 2.61f, 15.79f, 0.91f, 12.00f, 0.30f}
    };

    private static class PolyIcon extends Drawable {
        private final float[][] polys;
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final int size;
        private final Path path = new Path();

        PolyIcon(float[][] polys, int color, int size) {
            this.polys = polys;
            this.size = size;
            paint.setColor(color);
            paint.setStyle(Paint.Style.FILL);
            path.setFillType(Path.FillType.EVEN_ODD);
        }

        @Override
        public void draw(Canvas canvas) {
            Rect b = getBounds();
            float s = Math.min(b.width(), b.height()) / 24f;
            float ox = b.left + (b.width() - 24f * s) / 2f;
            float oy = b.top + (b.height() - 24f * s) / 2f;

            path.reset();

            for (int i = 0; i < polys.length; i++) {
                float[] p = polys[i];
                path.moveTo(ox + p[0] * s, oy + p[1] * s);

                for (int k = 2; k + 1 < p.length; k += 2) {
                    path.lineTo(ox + p[k] * s, oy + p[k + 1] * s);
                }

                path.close();
            }

            canvas.drawPath(path, paint);
        }

        @Override public int getIntrinsicWidth() { return size; }
        @Override public int getIntrinsicHeight() { return size; }
        @Override public void setAlpha(int alpha) { paint.setAlpha(alpha); }
        @Override public void setColorFilter(ColorFilter cf) { paint.setColorFilter(cf); }
        @Override public int getOpacity() { return PixelFormat.TRANSLUCENT; }
    }

    private static class YouTubeIcon extends Drawable {
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final int size;
        private final Path path = new Path();

        YouTubeIcon(int color, int size) {
            this.size = size;
            paint.setColor(color);
            paint.setStyle(Paint.Style.FILL);
            path.setFillType(Path.FillType.EVEN_ODD);
        }

        @Override
        public void draw(Canvas canvas) {
            Rect b = getBounds();
            float s = Math.min(b.width(), b.height()) / 24f;
            float ox = b.left + (b.width() - 24f * s) / 2f;
            float oy = b.top + (b.height() - 24f * s) / 2f;

            path.reset();

            path.addRoundRect(
                    ox + 1f * s, oy + 4.5f * s,
                    ox + 23f * s, oy + 19.5f * s,
                    4.5f * s, 4.5f * s,
                    Path.Direction.CW
            );

            path.moveTo(ox + 9.75f * s, oy + 8.6f * s);
            path.lineTo(ox + 15.75f * s, oy + 12f * s);
            path.lineTo(ox + 9.75f * s, oy + 15.4f * s);
            path.close();

            canvas.drawPath(path, paint);
        }

        @Override public int getIntrinsicWidth() { return size; }
        @Override public int getIntrinsicHeight() { return size; }
        @Override public void setAlpha(int alpha) { paint.setAlpha(alpha); }
        @Override public void setColorFilter(ColorFilter cf) { paint.setColorFilter(cf); }
        @Override public int getOpacity() { return PixelFormat.TRANSLUCENT; }
    }
}
