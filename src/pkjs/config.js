module.exports = [
  {
    "type": "heading",
    "defaultValue": "Boris Configuration"
  },
  {
    "type": "text",
    "defaultValue": "Enter your desired values below."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Weather (OpenWeatherMap)"
      },
      {
        "type": "input",
        "messageKey": "WeatherCity",
        "defaultValue": "",
        "label": "Show weather for city",
        "attributes": {
          "placeholder": "eg: Copenhagen",
          "limit": 32,
          "type": "text"
        }
      },
      {
        "type": "input",
        "messageKey": "WeatherKey",
        "defaultValue": "",
        "label": "OpenWeatherMap API key",
        "attributes": {
          "placeholder": "Get one at https://openweathermap.org/appid",
          "limit": 32,
          "type": "text"
        }
      }
    ]
  },
  {
    "type": "text",
    "defaultValue": "Enter your desired values below."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Sleep schedule"
      },
      {
        "type": "input",
        "messageKey": "Bedtime",
        "defaultValue": "",
        "label": "Go to sleep at",
        "attributes": {
          "placeholder": "HH:MM",
          "limit": 5,
          "type": "text"
        }
      },
      {
        "type": "input",
        "messageKey": "GetUpTime",
        "defaultValue": "",
        "label": "Get up at",
        "attributes": {
          "placeholder": "HH:MM",
          "limit": 5,
          "type": "text"
        }
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Look & feel"
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "label": "Background color",
        "defaultValue": "0x000000"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];