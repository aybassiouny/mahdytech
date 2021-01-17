import Typography from "typography"
import Wordpress2016 from "typography-theme-wordpress-2016"
import "@fontsource/open-sans/400.css" 

Wordpress2016.overrideThemeStyles = () => {
  return {
    "a.gatsby-resp-image-link": {
      boxShadow: `none`,
    },
  }
}

delete Wordpress2016.googleFonts
Wordpress2016.headerFontFamily = ["Open Sans"]
Wordpress2016.bodyFontFamily = ["Open Sans"]
Wordpress2016.baseFontSize = "18px"
const Wordpress2016copy = Object.assign({}, Wordpress2016);
Wordpress2016.overrideStyles =  (_ref, options) => ({
  ...Wordpress2016copy.overrideStyles(_ref, options),
  'ul,ol': {
    marginLeft: "50px",
  },
})
const typography = new Typography(Wordpress2016)

// Hot reload typography in development.
if (process.env.NODE_ENV !== `production`) {
  typography.injectStyles()
}

export default typography
export const rhythm = typography.rhythm
export const scale = typography.scale
