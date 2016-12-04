module.exports = [
  {
    "type": "heading",
    "defaultValue": "PebblGlobe Configuration"
  },
  {
    "type": "text",
    "defaultValue": "Modify the appearance."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Appearance"
      },
      {
        "type": "toggle",
        "messageKey": "Inverted",
        "defaultValue": false,
        "label": "Inverted Colors"
      },
      {
        "type": "toggle",
        "messageKey": "Bold",
        "label": "Use Bold Fonts",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "Center",
        "label": "Place Time in the Center",
        "defaultValue": false
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Features"
      },
      {
        "type": "toggle",
        "messageKey": "Animations",
        "label": "Enable Animations on shake",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "ShowDate",
        "label": "Show Date",
        "defaultValue": true
       },
      {
        "type": "toggle",
        "messageKey": "ShowHealth",
        "label": "Show Health Info",
        "defaultValue": true
       },
      {
        "type": "toggle",
        "messageKey": "ShowBattery",
        "label": "Show Battery Status",
        "defaultValue": true
       }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
