{
  'targets': [
    {
      'target_name': 'slp',
      'sources': [
        'src/node_slp.cc',
        'src/api/baton.cc',
        'src/api/findsrvs.cc',
        'src/api/findsrvtypes.cc',
        'src/api/findattrs.cc',
        'src/api/reg.cc',
        'src/api/dereg.cc',
        'src/api/delattrs.cc'
      ],
      'libraries': [
        '-lslp'
      ]
    }
  ]
}