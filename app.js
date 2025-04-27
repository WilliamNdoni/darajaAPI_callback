// Importing required modules
const express = require('express');
const mysql = require('mysql2');
const mqtt = require('mqtt');
require('dotenv').config();
// Add this near the top of your server file, after require statements
// const cors = require('cors');
// app.use(cors({
//   origin: 'http://localhost:3000', // or your frontend URL
//   methods: ['GET', 'POST', 'OPTIONS'],
//   allowedHeaders: ['Content-Type']
// }));
const cors = require('cors');
app.use(cors());
// Initializing the app
const app = express();

// Configuring middleware for JSON
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// MQTT Setup
const mqttOptions = {
  host: '4761eba0b4eb4a958f76528215b690db.s1.eu.hivemq.cloud',
  port: process.env.MQTT_PORT || 8883,
  protocol: 'mqtts',
  clientId: `mpesa_server_${Math.random().toString(16).slice(2, 8)}`,
  username: process.env.MQTT_USERNAME,
  password: process.env.MQTT_PASSWORD,
  keepalive: 600,
  reconnectPeriod: 1000,
  connectTimeout: 30 * 1000,
  clean: false
};

const client = mqtt.connect(mqttOptions);
const topic = 'wuzu58mpesa_data';

client.on('connect', () => {
  console.log('✅ Connected to MQTT broker');
});

client.on('error', (err) => {
  console.error('❌ MQTT connection error:', err);
});

// Persistent MySQL DB connection setup
let db;

function handleDisconnect() {
  db = mysql.createConnection({
    host: process.env.DB_HOST,
    user: process.env.DB_USER,
    password: process.env.DB_PASSWORD,
    database: process.env.DB_NAME,
    port: process.env.DB_PORT || 3306
  });

  db.connect((err) => {
    if (err) {
      console.error('❌ Error connecting to DB:', err.message);
      setTimeout(handleDisconnect, 2000); // Retry after delay
    } else {
      console.log('✅ Connected to the database.');
    }
  });

  db.on('error', (err) => {
    console.error('⚠️ DB Error:', err.message);
    if (err.code === 'PROTOCOL_CONNECTION_LOST' || err.fatal) {
      console.log('🔄 Reconnecting to the database...');
      handleDisconnect();
    } else {
      throw err;
    }
  });
}

// Call the DB connection handler
handleDisconnect();

// Optional: Keep connection alive by pinging every 60 seconds
setInterval(() => {
  if (db) {
    db.ping((err) => {
      if (err) {
        console.error('❌ DB ping error:', err.message);
      } else {
        console.log('📶 DB ping successful');
      }
    });
  }
}, 60000);

// M-Pesa Confirmation Endpoint
app.post('/milk_confirmation', (req, res) => {
  const data = req.body;
  const transaction_id = data.TransID;
  const transaction_time = data.TransTime;
  const amount = data.TransAmount;
  const phone_no = data.MSISDN;
  const fname = data.FirstName || "Unknown";
  const mname = data.MiddleName || "Unknown";
  const lname = data.LastName || "Unknown";

  console.log(fname, mname, phone_no, transaction_id, transaction_time);

  const litres = amount / 60;
  const pumpRunTimeMs = (litres / 0.02) * 1000;

  const insertQuery = `INSERT INTO payments (transaction_id, amount, phone, fname, mname, lname) VALUES (?, ?, ?, ?, ?, ?)`;

  db.query(insertQuery, [transaction_id, amount, phone_no, fname, mname, lname], (err, result) => {
    if (err) {
      console.error('❌ Error inserting data:', err.message);
      return res.status(500).json({ ResultCode: "1", ResultDesc: "Error storing payment" });
    }

    console.log('✅ Payment stored successfully.');
    client.publish(topic, JSON.stringify({
      first_name: fname,
      middle_name: mname,
      last_name: lname,
      amount: amount,
      pump_time_ms: Math.round(pumpRunTimeMs)
    }), { qos: 1 }, (err) => {
      if (err) {
        console.log('❌ Error publishing to MQTT:', err.message);
      } else {
        console.log('📡 Transaction data published to MQTT');
      }
    });

    res.status(200).json({
      ResultCode: "0",
      ResultDesc: "Success"
    });
  });
});

// M-Pesa Validation Endpoint
app.post('/milk_validation', (req, res) => {
  const transaction_id = req.body.TransID;
  console.log('🔍 Validation for transaction:', transaction_id);
  res.status(200).json({
    ResultCode: "0",
    ResultDesc: "Accepted"
  });
});

// Endpoint to retrieve all payment records
app.get('/payments', (req, res) => {
  const query = `SELECT * FROM payments`;

  db.query(query, (err, results) => {
    if (err) {
      console.error('❌ Error fetching data:', err.message);
      return res.status(500).json({ error: 'Database query failed' });
    }

    res.status(200).json(results);
  });
});

// Start the server
const PORT = process.env.PORT || 5000;
app.listen(PORT, () => {
  console.log(`🚀 Server running on port ${PORT}...`);
});
