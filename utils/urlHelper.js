/**
 * @param {string} base 
 * @param {Object} params 
 */
function buildUrl(base, params) {
    const url = new URL(base);
    Object.entries(params).forEach(([key, value]) => {
      url.searchParams.append(key, value);
    });
    return url.toString();
}

module.exports = {
    buildUrl
}
  