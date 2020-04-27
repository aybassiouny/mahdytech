import React from "react"
import { Link, graphql } from "gatsby"

import Bio from "../components/bio"
import Layout from "../components/layout"
import SEO from "../components/seo"
import { rhythm, scale } from "../utils/typography";

import customChart from '../components/custom-chart';
import { Input, Button, InputLabel, TextareaAutosize, TextField } from '@material-ui/core';
import InputAdornment from '@material-ui/core/InputAdornment';
import AccountCircle from '@material-ui/icons/AccountCircle';

class BlogPostTemplate extends React.Component {
  componentDidMount() {
    customChart();
  }

  render() {
    const post = this.props.data.markdownRemark
    const siteTitle = this.props.data.site.siteMetadata.title
    const { previous, next } = this.props.pageContext
    const image = post.frontmatter.socialPic? post.frontmatter.socialPic.childImageSharp.sizes.src : null

    return (
      <Layout location={this.props.location} title={siteTitle}>
        <SEO
          title={post.frontmatter.seotitle || post.frontmatter.title}
          description={post.frontmatter.description || post.excerpt}
          socialPic={image}
        />
        <h1>{post.frontmatter.title}</h1>
        <p
          style={{
            ...scale(-1 / 5),
            display: `block`,
            marginBottom: rhythm(1),
            marginTop: rhythm(-1),
          }}
        >
          {post.frontmatter.date}
        </p>
        <div dangerouslySetInnerHTML={{ __html: post.html }} />
        <hr />

        <form name="contact" method="POST" data-netlify="true" data-netlify-honeypot="bot-field">
          <input name="path" type="hidden" value={post.frontmatter.title} />
          <input type="hidden" name="form-name" value="contact" />
          <p style={{
              marginBottom: rhythm(0.5),
            }}>Leave a comment:</p>
          <div>
            <TextField label="Name" InputProps={{
          startAdornment: (
            <InputAdornment position="start">
              <AccountCircle />
            </InputAdornment>
          ),
        }} />
          </div>
          <div>
            <TextField label="Email" input="email" />
          {/* </div>
          <div> */}
            <TextField label="Message" fullWidth multiline />
          </div>
          <div  style={{
              marginTop: rhythm(.5),
            }}>
            <Button variant="outlined" type="submit" color="primary">Send</Button>
          </div>
        </form>
        <hr style={{
              marginTop: rhythm(1.5),
            }} />
        <Bio />

        <ul
          style={{
            display: `flex`,
            flexWrap: `wrap`,
            justifyContent: `space-between`,
            listStyle: `none`,
            padding: 0,
          }}
        >
          <li>
            {previous && (
              <Link to={previous.fields.slug} rel="prev">
                ← {previous.frontmatter.title}
              </Link>
            )}
          </li>
          <li>
            {next && (
              <Link to={next.fields.slug} rel="next">
                {next.frontmatter.title} →
              </Link>
            )}
          </li>
        </ul>
      </Layout>
    )
  }
}

export default BlogPostTemplate

export const pageQuery = graphql`
  query BlogPostBySlug($slug: String!) {
    site {
      siteMetadata {
        title
        author
      }
    }
    markdownRemark(fields: { slug: { eq: $slug } }) {
      id
      excerpt(pruneLength: 160)
      html
      frontmatter {
        title
        date(formatString: "MMMM DD, YYYY")
        description
        seotitle
        socialPic {
          childImageSharp {
            sizes(maxWidth: 400) {
              ...GatsbyImageSharpSizes_tracedSVG
            }
          }
          extension
          publicURL
        }
      }
    }
  }
`
