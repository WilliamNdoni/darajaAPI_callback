// Importing required modules
const express = require('express');
const mysql = require('mysql2');
require('dotenv').config();

// Initializing the app
const app = express();

// Configuring middleware for JSON
app.use(express.json());

// Create a database connection
const db = mysql.createConnection({
  host: process.env.DB_HOST,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  database: process.env.DB_NAME,
  port: process.env.DB_PORT || 3306
});

// Connect to the database
db.connect((err) => {
  if (err) {
    console.error('❌ Database connection failed:', err.message);
    return;
  }
  console.log('✅ Connected to the database.');
});

// M-Pesa Confirmation Endpoint (Insert Data into Database)
app.post('/mpesa_confirmation', (req, res) => {
  const data = req.body;
  const transaction_id = data.TransID;
  const transaction_time = data.TransTime;
  const amount = data.TransAmount;
  const phone_no = data.MSISDN;
  const fname = data.FirstName;
  const mname = data.MiddleName;

  console.log(fname, mname, phone_no, transaction_id, transaction_time);

  // Store transaction in database
  const insertQuery = `INSERT INTO payments (transaction_id, amount, phone, fname, mname) VALUES (?, ?, ?, ?, ?)`;

  db.query(insertQuery, [transaction_id, amount, phone_no, fname, mname], (err, result) => {
    if (err) {
      console.error('❌ Error inserting data:', err.message);
      return res.status(500).json({ ResultCode: "1", ResultDesc: "Error storing payment" });
    }

    console.log('✅ Payment stored successfully.');
    res.status(200).json({
      ResultCode: "0",
      ResultDesc: "Success"
    });
  });
});

// M-Pesa Validation Endpoint
app.post('/mpesa_validation', (req, res) => {
  const dataVal = req.body;
  const transaction_id = dataVal.TransID;
  console.log(transaction_id);
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
