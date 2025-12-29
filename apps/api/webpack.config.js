const { composePlugins, withNx } = require('@nx/webpack');
const path = require('path');

module.exports = composePlugins(withNx(), (config) => {
  // Only include files from the api app directory
  config.module.rules = config.module.rules.map((rule) => {
    if (rule.test && rule.test.toString().includes('ts')) {
      return {
        ...rule,
        include: [
          path.resolve(__dirname, 'src'),
          path.resolve(__dirname, '../../packages/shared'),
        ],
      };
    }
    return rule;
  });
  return config;
});
