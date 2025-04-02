const mysql = require('mysql2');
require('dotenv').config();

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

  // Create the payments table
  const createTableQuery = `
    CREATE TABLE IF NOT EXISTS payments (
      id INT AUTO_INCREMENT PRIMARY KEY,
      transaction_id VARCHAR(255) UNIQUE,
      amount DECIMAL(10,2),
      phone VARCHAR(20),
      fname VARCHAR(50),
      mname VARCHAR(50),
      lname VARCHAR(50),
      timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )`;

  db.query(createTableQuery, (err, result) => {
    if (err) {
      console.error('❌ Error creating table:', err.message);
    } else {
      console.log('✅ Table "payments" created successfully.');
    }
    db.end(); // Close the connection after executing the query
  });
});
