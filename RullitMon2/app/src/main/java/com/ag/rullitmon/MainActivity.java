/*
 * Copyright (c) 2011-2012 Jeff Boody
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

package com.ag.rullitmon;

import android.app.Activity;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.content.Intent;
import android.util.Log;
import android.os.Looper;
import android.os.Handler;
import android.os.Message;
import java.util.Set;
import java.util.ArrayList;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.widget.ToggleButton;

import androidx.annotation.NonNull;

import com.ag.rullitmon.BlueSmirfSPP;

public class MainActivity extends Activity implements Runnable, Handler.Callback, OnItemSelectedListener, View.OnTouchListener
{
	private static final String TAG = "Rullit";
	private static final int COMMAND_NONE = 0;
	private static final int COMMAND_START = 1;
	private static final int COMMAND_STOP = 2;
	private static final int COMMAND_LEFT = 3;
	private static final int COMMAND_LEFT_UNWIND = 3;
	private static final int COMMAND_UP = 4;
	private static final int COMMAND_LEFT_WIND = 4;
	private static final int COMMAND_DOWN = 5;
	private static final int COMMAND_RIGHT_UNWIND = 5;
	private static final int COMMAND_RIGHT = 6;
	private static final int COMMAND_RIGHT_WIND = 6;

	private static final int APPSTATUS_STARTUP = 0;
	private static final int APPSTATUS_HOMING = 4;
	private static final int APPSTATUS_COMPLETED = 5;

	private static final String[] APP_STATUSES = {
		"StartUp",
		"Moving",
		"Completed"
	};

	// app state
	private BlueSmirfSPP      mSPP;
	private boolean           mIsThreadRunning;
	private String            mBluetoothAddress;
	private ArrayList<String> mArrayListBluetoothAddress;

	// UI
	private TextView     mTextViewStatus;
	private TextView     mTextViewAppStatus;
	private TextView     mTextPosition;
	private TextView     mTextLengths;
	private TextView     mTextPWMs;
	private Spinner      mSpinnerDevices;
	private ArrayAdapter mArrayAdapterDevices;
	private Handler      mHandler;

	// Arduino state
	public boolean mManual;
	public int mCommand;
	public boolean mRollerStatus;
	public boolean mRollerCommand;
	public boolean mValveStatus;
	public boolean mValveCommand;
	public boolean mLeftSwitch;
	public boolean mRightSwitch;
	public boolean mFrontSwitch;
	public boolean mRearSwitch;
	public int mAppStatus;
	public int mPosX;
	public int mPosY;
	public int mALength;
	public int mBLength;
	public int mPWMLeft;
	public int mPWMRight;

	public MainActivity()
	{
		mIsThreadRunning           = false;
		mBluetoothAddress          = null;
		mSPP                       = new BlueSmirfSPP();
		mManual = false;
		mCommand = COMMAND_NONE;
		mRollerStatus = false;
		mRollerCommand = false;
		mValveStatus = false;
		mLeftSwitch = false;;
		mValveCommand = false;
		mRightSwitch= false;
		mFrontSwitch= false;
		mRearSwitch= false;
		mPosX = 0;
		mPosY = 0;
		mALength = 0;
		mBLength = 0;
		mPWMLeft = 0;
		mPWMRight = 0;

		mArrayListBluetoothAddress = new ArrayList<String>();
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// initialize UI
		setContentView(R.layout.activity_main);
		mTextViewStatus = (TextView) findViewById(R.id.status);
		mTextViewAppStatus = (TextView) findViewById(R.id.appStatus);
		mTextPosition = (TextView) findViewById(R.id.pos);
		mTextLengths = (TextView) findViewById(R.id.lengths);
		mTextPWMs = (TextView) findViewById(R.id.pwms);

		ArrayList<String> items = new ArrayList<String>();
		mSpinnerDevices         = (Spinner) findViewById(R.id.pairedDevices);
		mArrayAdapterDevices    = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, items);
		mHandler                = new Handler(this);
		mSpinnerDevices.setOnItemSelectedListener(this);
		mArrayAdapterDevices.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		mSpinnerDevices.setAdapter(mArrayAdapterDevices);

		findViewById(R.id.arrowLeft).setOnTouchListener(this);
		findViewById(R.id.arrowRight).setOnTouchListener(this);
		findViewById(R.id.arrowUp).setOnTouchListener(this);
		findViewById(R.id.arrowDown).setOnTouchListener(this);
		findViewById(R.id.arrowStop).setOnTouchListener(this);
		findViewById(R.id.arrowLeftUnwind).setOnTouchListener(this);
		findViewById(R.id.arrowLeftWind).setOnTouchListener(this);
		findViewById(R.id.arrowRightUnwind).setOnTouchListener(this);
		findViewById(R.id.arrowRightWind).setOnTouchListener(this);
	}

	@Override
	protected void onResume()
	{
		super.onResume();

		// update the paired device(s)
		BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
		Set<BluetoothDevice> devices = adapter.getBondedDevices();
		mArrayAdapterDevices.clear();
		mArrayListBluetoothAddress.clear();
		if(devices.size() > 0)
		{
			for(BluetoothDevice device : devices)
			{
				mArrayAdapterDevices.add(device.getName());
				mArrayListBluetoothAddress.add(device.getAddress());
			}

			// request that the user selects a device
			if(mBluetoothAddress == null)
			{
				//mSpinnerDevices.performClick();
			}
		}
		else
		{
			mBluetoothAddress = null;
		}

		UpdateUI();
	}

	@Override
	protected void onPause()
	{
		mSPP.disconnect();
		super.onPause();
	}

	@Override
	protected void onDestroy()
	{
		super.onDestroy();
	}

	@Override
	public boolean onTouch(View view, MotionEvent event) {
		if (event.getAction() == MotionEvent.ACTION_UP)
			mCommand = COMMAND_NONE;
		else
			mCommand = Integer.parseInt(view.getTag().toString());

		return false;
	}

	/*
	 * Spinner callback
	 */

	public void onItemSelected(AdapterView<?> parent, View view, int pos, long id)
	{
		mBluetoothAddress = mArrayListBluetoothAddress.get(pos);
	}

	public void onNothingSelected(AdapterView<?> parent)
	{
		mBluetoothAddress = null;
	}

	/*
	 * buttons
	 */

	public void onBluetoothSettings(View view)
	{
		Intent i = new Intent(android.provider.Settings.ACTION_BLUETOOTH_SETTINGS);
		startActivity(i);
	}

	public void onConnectClick(View view)
	{
		if(mIsThreadRunning == false)
		{
			ToggleButton tb = (ToggleButton)findViewById(R.id.startCommand);
			tb.setChecked(false);

			mIsThreadRunning = true;
			UpdateUI();
			Thread t = new Thread(this);
			t.start();
		}
	}

	public void onDisconnectClick(View view)
	{
		mSPP.disconnect();
	}

	public void onManualClick(View view) {
		ToggleButton tb = (ToggleButton)view;
		mManual = tb.isChecked();
	}

	public void onStartClick(View view) {
		ToggleButton tb = (ToggleButton)view;
		mCommand = tb.isChecked()? COMMAND_START : COMMAND_STOP;
	}

	public void onValveClick(View view) {
		ToggleButton tb = (ToggleButton)view;
		mValveCommand = tb.isChecked();
	}

	public void onRollerClick(View view) {
		ToggleButton tb = (ToggleButton)view;
		mRollerCommand = tb.isChecked();
	}

	private boolean appIsRunning()
	{
		return ((mAppStatus != APPSTATUS_STARTUP) && (mAppStatus != APPSTATUS_HOMING) && (mAppStatus != APPSTATUS_COMPLETED));

	}
	/*
	 * main loop
	 */

	public void run()
	{
		Looper.prepare();
		mSPP.connect(mBluetoothAddress);
		while(mSPP.isConnected())
		{
			int outputs = 0x00;
			if (mRollerCommand) outputs |= 0x01;
			if (mValveCommand) outputs |= 0x02;

			mSPP.writeByte('$');
			mSPP.writeByte(mManual? 1 : 0);
			mSPP.writeByte(mCommand);
			mSPP.writeByte(outputs);
			mSPP.writeByte('#');
			mSPP.flush();

			mSPP.readByte();
			mAppStatus = mSPP.readByte();
			int  inputs = mSPP.readByte();
			mLeftSwitch = ((inputs & 0x01) != 0);
			mFrontSwitch = ((inputs & 0x02) != 0);
			mRearSwitch = ((inputs & 0x04) != 0);
			mRightSwitch = ((inputs & 0x08) != 0);

			outputs = mSPP.readByte();
			mRollerStatus = ((outputs & 0x01) != 0);
			mValveStatus = ((outputs & 0x02) != 0);

			mPosX = mSPP.readByte();
			mPosX = (mPosX << 8) | mSPP.readByte();
			mPosY = mSPP.readByte();
			mPosY = (mPosY << 8) | mSPP.readByte();
			mALength = mSPP.readByte();
			mALength = (mALength << 8) | mSPP.readByte();
			mBLength = mSPP.readByte();
			mBLength = (mBLength << 8) | mSPP.readByte();
			mPWMLeft = mSPP.readByte();
			if (mPWMLeft > 128)
				mPWMLeft = - (256 - mPWMLeft);
			mPWMRight = mSPP.readByte();
			if (mPWMRight > 128)
				mPWMRight = - (256 - mPWMRight);

			mSPP.readByte();

			/*
			if ((mCommand == COMMAND_START) && appIsRunning())
				mCommand = COMMAND_NONE;
			*/

			if(mSPP.isError())
			{
				mSPP.disconnect();
			}

			mHandler.sendEmptyMessage(0);

			// wait briefly before sending the next packet
			try { Thread.sleep((long) (1000.0F/30.0F)); }
			catch(InterruptedException e) { Log.e(TAG, e.getMessage());}
		}

		mLeftSwitch = false;
		mRightSwitch = false;
		mFrontSwitch = false;
		mRearSwitch = false;
		mRollerStatus = false;
		mValveStatus = false;
		mPosX = 0;
		mPosY = 0;
		mALength = 0;
		mBLength = 0;

		mIsThreadRunning = false;
		mHandler.sendEmptyMessage(0);
	}

	/*
	 * update UI
	 */

	public boolean handleMessage (Message msg)
	{
		// received update request from Bluetooth IO thread
		UpdateUI();
		return true;
	}

	private void UpdateUI()
	{
		if(mSPP.isConnected())
		{
			mTextViewStatus.setText("connected to " + mSPP.getBluetoothAddress());
			mTextViewAppStatus.setText(APP_STATUSES[mAppStatus]);
			mTextPosition.setText(String.format("%d, %d", mPosX, mPosY));
			mTextLengths.setText(String.format("%d, %d", mALength, mBLength));
			mTextPWMs.setText(String.format("%d, %d %%", mPWMLeft, mPWMRight));

			ImageView frontSwitch = (ImageView)findViewById(R.id.switchFront);
			frontSwitch.setImageResource(mFrontSwitch? R.drawable.switch_hor_red : R.drawable.switch_hor_green);

			ImageView leftSwitch = (ImageView)findViewById(R.id.switchLeft);
			leftSwitch.setImageResource(mLeftSwitch? R.drawable.switch_vert_red : R.drawable.switch_vert_green);

			ImageView rightSwitch = (ImageView)findViewById(R.id.switchRight);
			rightSwitch.setImageResource(mRightSwitch? R.drawable.switch_vert_red : R.drawable.switch_vert_green);

			ImageView rearSwitch = (ImageView)findViewById(R.id.switchRear);
			rearSwitch.setImageResource(mRearSwitch? R.drawable.switch_hor_red : R.drawable.switch_hor_green);

			ToggleButton rollerButton = (ToggleButton)findViewById(R.id.rollerCommand);
			rollerButton.setChecked(mRollerStatus);

			ToggleButton valveButton = (ToggleButton)findViewById(R.id.valveCommand);
			valveButton.setChecked(mValveStatus);
		}
		else if(mIsThreadRunning)
		{
			mTextViewStatus.setText("connecting to " + mBluetoothAddress);
		}
		else
		{
			mTextViewStatus.setText("disconnected");
		}
	}
}
