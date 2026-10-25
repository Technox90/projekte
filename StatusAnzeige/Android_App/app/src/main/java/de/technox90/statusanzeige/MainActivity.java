package de.technox90.statusanzeige;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.*;
import android.bluetooth.le.*;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.*;
import android.view.Gravity;
import android.widget.*;
import java.nio.charset.StandardCharsets;
import java.util.UUID;

public class MainActivity extends Activity {
    private static final String DEVICE_NAME = "StatusAnzeige";
    private static final UUID SERVICE_UUID = UUID.fromString("7b4e1000-8b6f-4d69-9d52-5f6a7a8c1000");
    private static final UUID WRITE_UUID = UUID.fromString("7b4e1001-8b6f-4d69-9d52-5f6a7a8c1000");
    private static final UUID READ_UUID = UUID.fromString("7b4e1002-8b6f-4d69-9d52-5f6a7a8c1000");
    private static final UUID CCCD_UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    private static final int REQ_PERMISSIONS = 1001;

    private final Handler handler = new Handler(Looper.getMainLooper());
    private BluetoothAdapter bluetoothAdapter;
    private BluetoothLeScanner scanner;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic writeCharacteristic;
    private BluetoothGattCharacteristic readCharacteristic;
    private TextView connectionText, deviceText, stateText;
    private Button connectButton, disconnectButton;
    private boolean scanning=false, connected=false;

    @Override public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        BluetoothManager manager=(BluetoothManager)getSystemService(BLUETOOTH_SERVICE);
        bluetoothAdapter=manager.getAdapter();
        buildUi();
        if(bluetoothAdapter==null){
            setConnection("Bluetooth wird nicht unterstützt.",false);
            connectButton.setEnabled(false);
            return;
        }
        requestBluetoothPermissionsIfNeeded();
    }

    private void buildUi(){
        int bg=Color.rgb(8,11,16), card=Color.rgb(18,24,34), primary=Color.rgb(31,111,235),
            secondary=Color.rgb(40,50,65), text=Color.rgb(238,242,247), muted=Color.rgb(154,168,186);
        ScrollView scroll=new ScrollView(this); scroll.setBackgroundColor(bg);
        LinearLayout root=new LinearLayout(this); root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(18),dp(18),dp(18),dp(24)); scroll.addView(root);

        TextView title=new TextView(this); title.setText("StatusAnzeige BT"); title.setTextSize(28);
        title.setTextColor(text); title.setTypeface(null,android.graphics.Typeface.BOLD); root.addView(title);
        TextView subtitle=new TextView(this); subtitle.setText("Bluetooth-Steuerung für deine StatusAnzeige");
        subtitle.setTextColor(muted); subtitle.setTextSize(14); subtitle.setPadding(0,dp(4),0,dp(18)); root.addView(subtitle);

        LinearLayout connectionCard=card(card); root.addView(connectionCard);
        connectionText=new TextView(this); connectionText.setText("Nicht verbunden");
        connectionText.setTextColor(Color.rgb(255,107,107)); connectionText.setTextSize(18);
        connectionText.setTypeface(null,android.graphics.Typeface.BOLD); connectionCard.addView(connectionText);

        deviceText=new TextView(this); deviceText.setText("Gerät: –"); deviceText.setTextColor(muted);
        deviceText.setPadding(0,dp(6),0,dp(12)); connectionCard.addView(deviceText);

        LinearLayout connectionButtons=new LinearLayout(this); connectionButtons.setOrientation(LinearLayout.HORIZONTAL);
        connectionButtons.setGravity(Gravity.CENTER_VERTICAL); connectionCard.addView(connectionButtons);

        connectButton=button("Suchen & Verbinden",primary);
        connectionButtons.addView(connectButton,new LinearLayout.LayoutParams(0,dp(50),1f));
        disconnectButton=button("Trennen",secondary);
        LinearLayout.LayoutParams dp2=new LinearLayout.LayoutParams(0,dp(50),0.55f); dp2.setMargins(dp(8),0,0,0);
        connectionButtons.addView(disconnectButton,dp2); disconnectButton.setEnabled(false);
        connectButton.setOnClickListener(v->startScan()); disconnectButton.setOnClickListener(v->disconnectGatt());

        LinearLayout statusCard=card(card); root.addView(statusCard);
        TextView statusTitle=new TextView(this); statusTitle.setText("Schnellstatus"); statusTitle.setTextColor(text);
        statusTitle.setTextSize(20); statusTitle.setTypeface(null,android.graphics.Typeface.BOLD);
        statusTitle.setPadding(0,0,0,dp(12)); statusCard.addView(statusTitle);

        GridLayout grid=new GridLayout(this); grid.setColumnCount(2); statusCard.addView(grid);
        addStatusButton(grid,"FREI","status:frei");
        addStatusButton(grid,"BUSSY","status:busy");
        addStatusButton(grid,"MEETING","status:meeting");
        addStatusButton(grid,"PAUSE","status:pause");
        addStatusButton(grid,"SENDE EMAIL","status:mail");
        addStatusButton(grid,"BITTE WARTEN","status:bitte_warten");

        LinearLayout infoCard=card(card); root.addView(infoCard);
        TextView infoTitle=new TextView(this); infoTitle.setText("Gerätestatus"); infoTitle.setTextColor(text);
        infoTitle.setTextSize(18); infoTitle.setTypeface(null,android.graphics.Typeface.BOLD); infoCard.addView(infoTitle);
        stateText=new TextView(this); stateText.setText("Noch keine Gerätedaten empfangen.");
        stateText.setTextColor(muted); stateText.setTextSize(14); stateText.setPadding(0,dp(8),0,0); infoCard.addView(stateText);
        setContentView(scroll);
    }

    private LinearLayout card(int color){
        LinearLayout c=new LinearLayout(this); c.setOrientation(LinearLayout.VERTICAL);
        c.setPadding(dp(16),dp(16),dp(16),dp(16)); c.setBackgroundColor(color);
        LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,-2); p.setMargins(0,0,0,dp(14)); c.setLayoutParams(p); return c;
    }
    private Button button(String label,int color){
        Button b=new Button(this); b.setText(label); b.setTextColor(Color.WHITE); b.setTextSize(14);
        b.setAllCaps(false); b.setBackgroundColor(color); return b;
    }
    private void addStatusButton(GridLayout grid,String label,String command){
        Button b=button(label,Color.rgb(24,33,48)); b.setTextSize(16);
        b.setTypeface(null,android.graphics.Typeface.BOLD); b.setOnClickListener(v->sendCommand(command));
        GridLayout.LayoutParams p=new GridLayout.LayoutParams(); p.width=0; p.height=dp(72);
        p.columnSpec=GridLayout.spec(GridLayout.UNDEFINED,1f); p.setMargins(dp(4),dp(4),dp(4),dp(4)); grid.addView(b,p);
    }

    private void requestBluetoothPermissionsIfNeeded(){
        if(Build.VERSION.SDK_INT>=Build.VERSION_CODES.S){
            if(checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN)!=PackageManager.PERMISSION_GRANTED ||
               checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)!=PackageManager.PERMISSION_GRANTED){
                requestPermissions(new String[]{Manifest.permission.BLUETOOTH_SCAN,Manifest.permission.BLUETOOTH_CONNECT},REQ_PERMISSIONS);
            }
        } else if(checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION)!=PackageManager.PERMISSION_GRANTED){
            requestPermissions(new String[]{Manifest.permission.ACCESS_FINE_LOCATION},REQ_PERMISSIONS);
        }
    }

    private boolean hasBluetoothPermissions(){
        if(Build.VERSION.SDK_INT>=Build.VERSION_CODES.S){
            return checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN)==PackageManager.PERMISSION_GRANTED &&
                   checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)==PackageManager.PERMISSION_GRANTED;
        }
        return checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION)==PackageManager.PERMISSION_GRANTED;
    }

    @Override public void onRequestPermissionsResult(int requestCode,String[] permissions,int[] grantResults){
        super.onRequestPermissionsResult(requestCode,permissions,grantResults);
        if(requestCode==REQ_PERMISSIONS && !hasBluetoothPermissions()) setConnection("Bluetooth-Berechtigungen fehlen.",false);
    }

    private void startScan(){
        if(!hasBluetoothPermissions()){ requestBluetoothPermissionsIfNeeded(); return; }
        if(!bluetoothAdapter.isEnabled()){ Toast.makeText(this,"Bitte Bluetooth einschalten.",Toast.LENGTH_LONG).show(); return; }
        if(connected){ Toast.makeText(this,"StatusAnzeige ist bereits verbunden.",Toast.LENGTH_SHORT).show(); return; }
        scanner=bluetoothAdapter.getBluetoothLeScanner();
        if(scanner==null){ setConnection("BLE-Scanner nicht verfügbar.",false); return; }
        if(scanning) return;
        scanning=true; connectButton.setEnabled(false); setConnection("Suche StatusAnzeige …",false); deviceText.setText("Gerät: Suche läuft");
        scanner.startScan(scanCallback);
        handler.postDelayed(()->{
            if(scanning){ stopScan(); setConnection("StatusAnzeige nicht gefunden.",false); connectButton.setEnabled(true); deviceText.setText("Gerät: –"); }
        },12000);
    }

    private void stopScan(){
        if(!scanning||scanner==null) return;
        if(hasBluetoothPermissions()) scanner.stopScan(scanCallback);
        scanning=false;
    }

    private final ScanCallback scanCallback=new ScanCallback(){
        @Override public void onScanResult(int callbackType,ScanResult result){
            BluetoothDevice device=result.getDevice(); if(!hasBluetoothPermissions()) return;
            String name=null;
            try{name=device.getName();}catch(SecurityException ignored){}
            if(name==null && result.getScanRecord()!=null) name=result.getScanRecord().getDeviceName();
            if(name!=null && name.toLowerCase().contains(DEVICE_NAME.toLowerCase())){ stopScan(); connectTo(device); }
        }
        @Override public void onScanFailed(int errorCode){
            scanning=false; runOnUiThread(()->{ connectButton.setEnabled(true); setConnection("BLE-Scan fehlgeschlagen: "+errorCode,false); });
        }
    };

    private void connectTo(BluetoothDevice device){
        if(!hasBluetoothPermissions()) return;
        setConnection("Verbinde …",false); deviceText.setText("Gerät: "+safeDeviceName(device));
        gatt=device.connectGatt(this,false,gattCallback,BluetoothDevice.TRANSPORT_LE);
    }

    private String safeDeviceName(BluetoothDevice device){
        try{ String n=device.getName(); return n!=null?n:device.getAddress(); }catch(SecurityException e){ return "StatusAnzeige"; }
    }

    private final BluetoothGattCallback gattCallback=new BluetoothGattCallback(){
        @Override public void onConnectionStateChange(BluetoothGatt g,int status,int newState){
            if(newState==BluetoothProfile.STATE_CONNECTED){
                connected=true; runOnUiThread(()->{ setConnection("Verbunden – lade Dienste …",true); connectButton.setEnabled(false); disconnectButton.setEnabled(true); });
                if(hasBluetoothPermissions()) g.discoverServices();
            } else if(newState==BluetoothProfile.STATE_DISCONNECTED){
                connected=false; writeCharacteristic=null; readCharacteristic=null;
                runOnUiThread(()->{ setConnection("Nicht verbunden",false); connectButton.setEnabled(true); disconnectButton.setEnabled(false); deviceText.setText("Gerät: –"); });
            }
        }
        @Override public void onServicesDiscovered(BluetoothGatt g,int status){
            BluetoothGattService service=g.getService(SERVICE_UUID);
            if(service==null){ runOnUiThread(()->setConnection("BLE-Service nicht gefunden.",false)); return; }
            writeCharacteristic=service.getCharacteristic(WRITE_UUID); readCharacteristic=service.getCharacteristic(READ_UUID);
            if(writeCharacteristic==null){ runOnUiThread(()->setConnection("WRITE-Characteristic fehlt.",false)); return; }
            if(readCharacteristic!=null && hasBluetoothPermissions()){
                g.setCharacteristicNotification(readCharacteristic,true);
                BluetoothGattDescriptor d=readCharacteristic.getDescriptor(CCCD_UUID);
                if(d!=null){
                    if(Build.VERSION.SDK_INT>=33) g.writeDescriptor(d,BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                    else { d.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE); g.writeDescriptor(d); }
                }
            }
            runOnUiThread(()->setConnection("StatusAnzeige verbunden",true));
        }
        @Override public void onCharacteristicChanged(BluetoothGatt g,BluetoothGattCharacteristic c,byte[] value){
            if(READ_UUID.equals(c.getUuid())){
                String t=new String(value,StandardCharsets.UTF_8); runOnUiThread(()->stateText.setText(t));
            }
        }
        @SuppressWarnings("deprecation")
        @Override public void onCharacteristicChanged(BluetoothGatt g,BluetoothGattCharacteristic c){
            if(READ_UUID.equals(c.getUuid())){
                String t=new String(c.getValue(),StandardCharsets.UTF_8); runOnUiThread(()->stateText.setText(t));
            }
        }
    };

    @SuppressWarnings("deprecation")
    private void sendCommand(String command){
        if(!connected||gatt==null||writeCharacteristic==null){
            Toast.makeText(this,"Bitte zuerst verbinden.",Toast.LENGTH_SHORT).show(); return;
        }
        if(!hasBluetoothPermissions()) return;
        byte[] value=command.getBytes(StandardCharsets.UTF_8);
        int result;
        if(Build.VERSION.SDK_INT>=33){
            result=gatt.writeCharacteristic(writeCharacteristic,value,BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
        } else {
            writeCharacteristic.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
            writeCharacteristic.setValue(value);
            result=gatt.writeCharacteristic(writeCharacteristic)?0:-1;
        }
        if(result==0) stateText.setText("Gesendet: "+command);
        else Toast.makeText(this,"BLE-Befehl konnte nicht gesendet werden.",Toast.LENGTH_SHORT).show();
    }

    private void disconnectGatt(){
        stopScan();
        if(gatt!=null && hasBluetoothPermissions()){ gatt.disconnect(); gatt.close(); gatt=null; }
        connected=false; writeCharacteristic=null; readCharacteristic=null;
        setConnection("Nicht verbunden",false); connectButton.setEnabled(true); disconnectButton.setEnabled(false); deviceText.setText("Gerät: –");
    }

    private void setConnection(String text,boolean ok){
        connectionText.setText(text); connectionText.setTextColor(ok?Color.rgb(98,217,139):Color.rgb(255,107,107));
    }
    private int dp(int value){ return Math.round(value*getResources().getDisplayMetrics().density); }
    @Override protected void onDestroy(){ disconnectGatt(); super.onDestroy(); }
}
