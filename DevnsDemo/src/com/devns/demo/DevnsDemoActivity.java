package com.devns.demo;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.view.View;
import android.view.View.OnClickListener;
import android.util.Log;

public class DevnsDemoActivity extends Activity {
	
	private static final String TAG = "DevnsDemo";
	private static final String DEVNS_FIFO = "/dev/devns.fifo";
	private static final byte SWITCH_TARGET = 1;

	Button button;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		addListenerOnButton();
	}

	public void addListenerOnButton() {
		button = (Button) findViewById(R.id.button1);
		button.setOnClickListener(new OnClickListener() {

		@Override
		public void onClick(View arg0) {
			Log.i(TAG, "Request switch to secondary namespace");
			try {
				File fifo = new File(DEVNS_FIFO);
				FileOutputStream out = new FileOutputStream(fifo);
				out.write(SWITCH_TARGET);
				out.close();
			} catch (IOException exception) {
				Log.e(TAG, "Failed accessing fifo: " + exception.getMessage(), exception);
			}
		}

		});

	}

}
