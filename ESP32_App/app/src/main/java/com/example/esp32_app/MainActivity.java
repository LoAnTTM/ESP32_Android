package com.example.esp32_app;

import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.switchmaterial.SwitchMaterial;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.Query;
import com.google.firebase.database.ValueEventListener;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

public class MainActivity extends AppCompatActivity {

    private TextView textViewTemp, textViewHum, textViewLastUpdated;
    private EditText editTextThreshold;
    private SwitchMaterial switchMode;
    private Button buttonSubmitThreshold;
    private DatabaseReference dhtDataRef, thresholdRef, modeRef;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textViewTemp = findViewById(R.id.textViewTemp);
        textViewHum = findViewById(R.id.textViewHum);
        textViewLastUpdated = findViewById(R.id.textViewLastUpdated);
        editTextThreshold = findViewById(R.id.editTextThreshold);
        switchMode = findViewById(R.id.switchMode);
        buttonSubmitThreshold = findViewById(R.id.buttonSubmitThreshold);

        FirebaseDatabase database = FirebaseDatabase.getInstance();
        dhtDataRef = database.getReference("DHT_Data");
        thresholdRef = database.getReference("Config/threshold");
        modeRef = database.getReference("Config/mode");

        setupListeners();
    }

    private void setupListeners() {
        // Query to get the latest data entry by ordering by key (timestamp)
        Query lastDataQuery = dhtDataRef.orderByKey().limitToLast(1);

        lastDataQuery.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                if (dataSnapshot.exists()) {
                    // Since we limited to 1, there's only one child
                    for (DataSnapshot childSnapshot : dataSnapshot.getChildren()) {
                        Float temperature = childSnapshot.child("temperature").getValue(Float.class);
                        Integer humidity = childSnapshot.child("humidity").getValue(Integer.class);
                        String timestampStr = childSnapshot.getKey();
                        if (timestampStr != null) {
                            try {
                                long timestamp = Long.parseLong(timestampStr);
                                updateTimestamp(timestamp);
                            } catch (NumberFormatException e) {
                                Log.e("MainActivity", "Invalid timestamp format: " + timestampStr, e);
                                textViewLastUpdated.setText("Last updated: Invalid timestamp");
                            }
                        }

                        if (temperature != null) {
                            textViewTemp.setText(String.format(Locale.US, "%.1f °C", temperature));
                        }
                        if (humidity != null) {
                            textViewHum.setText(String.format(Locale.US, "%d %%", humidity));
                        }
                    }
                }
            }

            @Override
            public void onCancelled(@NonNull DatabaseError databaseError) {
                textViewTemp.setText("Error");
                textViewHum.setText("Error");
            }
        });

        // Listen for threshold changes from Firebase to update the EditText
        thresholdRef.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                Float threshold = dataSnapshot.getValue(Float.class);
                if (threshold != null) {
                    editTextThreshold.setText(String.valueOf(threshold));
                }
            }
            @Override
            public void onCancelled(@NonNull DatabaseError error) {}
        });

        // Listen for mode changes from Firebase to update the Switch
        modeRef.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                String mode = dataSnapshot.getValue(String.class);
                if (mode != null) {
                    switchMode.setChecked("alert".equals(mode));
                }
            }
            @Override
            public void onCancelled(@NonNull DatabaseError error) {}
        });


        // Set threshold when the new button is clicked
        buttonSubmitThreshold.setOnClickListener(v -> {
            try {
                float threshold = Float.parseFloat(editTextThreshold.getText().toString());
                thresholdRef.setValue(threshold)
                        .addOnSuccessListener(aVoid -> Toast.makeText(MainActivity.this, "Threshold updated!", Toast.LENGTH_SHORT).show())
                        .addOnFailureListener(e -> Toast.makeText(MainActivity.this, "Failed to update.", Toast.LENGTH_SHORT).show());
            } catch (NumberFormatException e) {
                Toast.makeText(MainActivity.this, "Invalid number format", Toast.LENGTH_SHORT).show();
            }
        });

        // Set mode when switch is toggled
        switchMode.setOnCheckedChangeListener((buttonView, isChecked) -> {
            modeRef.setValue(isChecked ? "alert" : "normal");
        });
    }

    private void updateTimestamp(long unixSeconds) {
        Date date = new Date(unixSeconds * 1000L); // Convert seconds to milliseconds
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault());
        sdf.setTimeZone(TimeZone.getDefault()); // Use local timezone
        String formattedDate = sdf.format(date);
        textViewLastUpdated.setText("Last updated: " + formattedDate);
    }
}