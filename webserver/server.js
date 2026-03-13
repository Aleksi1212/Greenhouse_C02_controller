const express = require("express");
const path = require("path");
require('dotenv').config()

const app = express();

app.use(express.static(__dirname));

app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// ThingSpeak Configuration
const TALKBACK_ID = process.env.TALKBACK_ID;
const TALKBACK_API_KEY = process.env.TALKBACK_API_KEY;
const READ_API_KEY = process.env.READ_API_KEY;

app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "index.html"));
});

app.get("/data", async (req, res) => {
  try {
    const response = await fetch(`https://api.thingspeak.com/channels/3272963/feeds.json?api_key=${READ_API_KEY}&results=1`, {
      method: "GET",
    })
    const data = await response.json()
    res.json(data)
  } catch (err) {
    res.json({ error: true })
  }
})

app.post("/sendCommand", async (req, res) => {
  const command = req.body.command;

  try {
    const response = await fetch(`https://api.thingspeak.com/talkbacks/${TALKBACK_ID}/commands.json`, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: `api_key=${TALKBACK_API_KEY}&command_string=SET_CO2|${encodeURIComponent(command)}`
    });

    const data = await response.json();
    res.json(data);

  } catch (err) {
    console.error("Server Error:", err);
    res.status(500).json({ error: "Could not reach ThingSpeak" });
  }
});

const PORT = 3000;
app.listen(PORT, () => {
  console.log(`--- Greenhouse Server Active ---`);
  console.log(`Running at: http://localhost:${PORT}`);
});