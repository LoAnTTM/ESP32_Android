package com.example.esp32_app;

import android.os.Bundle;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.switchmaterial.SwitchMaterial;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.database.ValueEventListener;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

public class MainActivity extends AppCompatActivity {

    private TextView textViewTemp, textViewHum, textViewLastUpdated, textviewThreshold;
    private SwitchMaterial switchMode;
    private ImageButton buttonIncrement, buttonDecrement;
    private DatabaseReference latestRef, thresholdRef, modeRef;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Initialize UI components
        textViewTemp = findViewById(R.id.textViewTemp);
        textViewHum = findViewById(R.id.textViewHum);
        textViewLastUpdated = findViewById(R.id.textViewLastUpdated);
        textviewThreshold = findViewById(R.id.textViewThreshold);
        switchMode = findViewById(R.id.switchMode);
        buttonIncrement = findViewById(R.id.buttonIncrement);
        buttonDecrement = findViewById(R.id.buttonDecrement);

        // Initialize Firebase references
        FirebaseAuth auth = FirebaseAuth.getInstance();

        String email = "test@mail.com";
        String password = "test@1.";
        if (auth.getCurrentUser() == null) {
            auth.signInWithEmailAndPassword(email, password)
                    .addOnCompleteListener(task -> {
                        if (task.isSuccessful()) {
                            Toast.makeText(this, "Signed in automatically", Toast.LENGTH_SHORT).show();
                            initFirebaseReferences();
                            setupListeners();
                        } else {
                            Toast.makeText(this, "Login failed: " + task.getException().getMessage(), Toast.LENGTH_LONG).show();
                        }
                    });
        }else {
            initFirebaseReferences();
            setupListeners();
        }
    }

    private void setupListeners() {
        // Listen for latest temperature and humidity updates from Firebase
        latestRef.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                if (dataSnapshot.exists()) {
                    Float temperature = dataSnapshot.child("Temperature").getValue(Float.class);
                    Float humidity = dataSnapshot.child("Humidity").getValue(Float.class);
                    Long timestamp = dataSnapshot.child("Timestamp").getValue(Long.class);

                    if (timestamp != null) {
                        updateTimestamp(timestamp);
                    }

                    if (temperature != null) {
                        textViewTemp.setText(String.format(Locale.US, "%.1f °C", temperature));
                    }
                    if (humidity != null) {
                        textViewHum.setText(String.format(Locale.US, "%.1f %%", humidity));
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
                    textviewThreshold.setText(String.format(Locale.US, "%.1f", threshold));
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
            public void onCancelled(@NonNull DatabaseError error) {
                Toast.makeText(MainActivity.this, "Database error: " + error.getMessage(), Toast.LENGTH_SHORT).show();
            }

        });

        buttonIncrement.setOnClickListener(v -> updateThreshold(1.0f));
        buttonDecrement.setOnClickListener(v -> updateThreshold(-1.0f));

        // Set mode when switch is toggled
        switchMode.setOnCheckedChangeListener((buttonView, isChecked) -> {
            String newMode = isChecked ? "alert" : "normal";
            modeRef.get().addOnSuccessListener(snapshot -> {
                String currentMode = snapshot.getValue(String.class);
                if (!newMode.equals(currentMode)) {
                    modeRef.setValue(newMode)
                            .addOnSuccessListener(aVoid -> Toast.makeText(MainActivity.this, "Mode set to " + newMode, Toast.LENGTH_SHORT).show())
                            .addOnFailureListener(e -> Toast.makeText(MainActivity.this, "Failed to set mode.", Toast.LENGTH_SHORT).show());
                }
            });
        });
    }

    private void initFirebaseReferences() {
        FirebaseDatabase database = FirebaseDatabase.getInstance();
        latestRef = database.getReference("Config/Latest");
        thresholdRef = database.getReference("Config/Threshold");
        modeRef = database.getReference("Config/Mode");
    }

    // Update threshold value in Firebase
    private void updateThreshold(float delta) {
        try {
            float currentThreshold = Float.parseFloat(textviewThreshold.getText().toString());
            float newThreshold = currentThreshold + delta;
            thresholdRef.setValue(newThreshold)
                    .addOnSuccessListener(aVoid -> {
                        // The listener will update the TextView
                        textviewThreshold.setText(String.format(Locale.US, "%.1f", newThreshold));
                        Toast.makeText(MainActivity.this, "Threshold updated!", Toast.LENGTH_SHORT).show();
                    })
                    .addOnFailureListener(e -> Toast.makeText(MainActivity.this, "Failed to update.", Toast.LENGTH_SHORT).show());
        } catch (NumberFormatException e) {
            Toast.makeText(MainActivity.this, "Invalid number format", Toast.LENGTH_SHORT).show();
        }
    }

    // Convert Unix timestamp to human-readable format and update TextView
    private void updateTimestamp(long unixSeconds) {
        Date date = new Date(unixSeconds * 1000L); // Convert seconds to milliseconds
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault());
        sdf.setTimeZone(TimeZone.getDefault()); // Use local timezone
        String formattedDate = sdf.format(date);
        textViewLastUpdated.setText("Last updated: " + formattedDate);
    }
}