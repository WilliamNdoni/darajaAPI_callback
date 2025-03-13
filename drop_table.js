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

  // Drop the transactions table
  const dropTableQuery = `DROP TABLE IF EXISTS transactions`;

  db.query(dropTableQuery, (err, result) => {
    if (err) {
      console.error('❌ Error dropping table:', err.message);
    } else {
      console.log('✅ Table "transactions" deleted successfully.');
    }
    db.end(); // Close the connection after executing the query
  });
});
