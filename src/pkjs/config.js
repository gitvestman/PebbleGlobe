module.exports = [
  {
    "type": "heading",
    "defaultValue": "BB8 Configuration"
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
        "label": "White Background"
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
        "defaultValue": false
       }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
