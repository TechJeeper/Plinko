module.exports = [
  {
    type: 'heading',
    defaultValue: 'Plinko'
  },
  {
    type: 'text',
    defaultValue: 'Aim, drop, bounce — tune difficulty, balls, and look.'
  },

  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Gameplay'
      },
      {
        type: 'select',
        messageKey: 'Difficulty',
        defaultValue: '1',
        label: 'Difficulty',
        options: [
          { label: 'Easy — slower fall', value: '0' },
          { label: 'Normal', value: '1' },
          { label: 'Hard — faster & wilder', value: '2' }
        ]
      },
      {
        type: 'select',
        messageKey: 'BallsPerRound',
        defaultValue: '10',
        label: 'Balls per round',
        options: [
          { label: '5 balls', value: '5' },
          { label: '10 balls', value: '10' },
          { label: '15 balls', value: '15' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'Vibrate',
        label: 'Vibrate on pegs & scores',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'ShowAimGuide',
        label: 'Show aim drop guide',
        defaultValue: true
      }
    ]
  },

  {
    type: 'section',
    capabilities: ['COLOR'],
    items: [
      {
        type: 'heading',
        defaultValue: 'Colors'
      },
      {
        type: 'color',
        messageKey: 'BackgroundColor',
        defaultValue: '0x000055',
        label: 'Background',
        sunlight: false
      },
      {
        type: 'color',
        messageKey: 'PegColor',
        defaultValue: '0xAAAAAA',
        label: 'Pegs',
        sunlight: false
      },
      {
        type: 'color',
        messageKey: 'BallColor',
        defaultValue: '0xFFAA00',
        label: 'Ball',
        sunlight: false
      }
    ]
  },

  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
