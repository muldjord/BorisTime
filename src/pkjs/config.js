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
      },
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];