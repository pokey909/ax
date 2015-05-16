package com.gst_sdk_tutorials.tutorial_5;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import org.freedesktop.gstreamer.GStreamer;
import com.audiox.Player;
import android.widget.Toast;
import android.util.Log;
import android.widget.Spinner;
import android.widget.AdapterView;
import android.view.View;
import android.widget.ArrayAdapter;

public class Main3Activity extends Activity implements AdapterView.OnItemSelectedListener {
    private Player m_player;

    String[] items = {  "http://www.noiseaddicts.com/samples/281.mp3",
                        "http://www.noiseaddicts.com/samples/181.mp3",
                        "http://www.noiseaddicts.com/samples/4637.mp3",
                        "http://www.noiseaddicts.com/samples/3185.mp3",
                        "http://www.noiseaddicts.com/samples/3719.mp3" };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main3);
        try {
            GStreamer.init(this);
        } catch (Exception e) {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        m_player = new Player();

        Spinner spin = (Spinner) findViewById(R.id.spinner);
        spin.setOnItemSelectedListener(this);

        ArrayAdapter aa = new ArrayAdapter(
                this,
                android.R.layout.simple_spinner_item,
                items);
        aa.setDropDownViewResource(
                android.R.layout.simple_spinner_dropdown_item);
        spin.setAdapter(aa);
    }
    public void onItemSelected(AdapterView<?> parent, View v, int position, long id) {
        m_player.play(items[position]);
    }

    public void onNothingSelected(AdapterView<?> parent) {
        m_player.stop();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main3, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        m_player.play("http://www.noiseaddicts.com/samples/3925.mp3");
        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
    static {
        System.loadLibrary("gstreamer_android");
    }
}
