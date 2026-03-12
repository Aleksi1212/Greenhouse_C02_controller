const express = require("express");
const path = require("path");

const app = express();

app.use(express.static(__dirname));

app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// ThingSpeak Configuration
const TALKBACK_ID = "";
const API_KEY = "";

app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "index.html"));
});

app.post("/sendCommand", async (req, res) => {
  const command = req.body.command;

  try {
    const response = await fetch(`https://api.thingspeak.com/talkbacks/${TALKBACK_ID}/commands.json`, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: `api_key=${API_KEY}&command_string=SET_CO2|${encodeURIComponent(command)}`
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