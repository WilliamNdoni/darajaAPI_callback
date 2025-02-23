// Importing the relevant modules
const express = require('express');
const SerialPort = require('serialport');

// Initializing the app
const app = express();

// const NGROK_AUTHTOKEN= '2tQtCD4hpHoeS0Obnw4ehGbUn9r_3nfEepdSm1KT3HkrvjFHj';

// Configuring the middleware for json
app.use(express.json());

app.post('/boyz_confirmation',(req,res) => {
  const data = req.body
  const transaction_id = data.TransID;
  const transaction_time = data.TransTime;
  const phone_no = data.MSISDN;
  const fname = data.FirstName;
  const mname = data.MiddleName;
  console.log(fname, mname, phone_no, transaction_id, transaction_time);
  res.status(200).json({
    ResultCode: "0",
    ResultDesc: "Sucess"
  });
});

app.post('/boyz_validation', (req,res)=> {
  const dataVal = req.body;
  const transaction_id = dataVal.TransID;
  console.log(transaction_id);
  res.status(200).json({
    ResultCode: "0",
    ResultDesc: "Accepted"
  });
});

app.listen(5000, () => {
  console.log('Server listening to port 5000...');
});

