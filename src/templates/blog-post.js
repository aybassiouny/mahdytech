import React from "react"
import { Link, graphql } from "gatsby"

import Bio from "../components/bio"
import Layout from "../components/layout"
import SEO from "../components/seo"
import { rhythm, scale } from "../utils/typography";
import "./blog-post.css";

import customChart from '../components/custom-chart';
import { Button, TextField } from '@material-ui/core';
import Typography from '@material-ui/core/Typography';
import CommentCard from '../utils/comment-card';

class BlogPostTemplate extends React.Component {
  componentDidMount() {
    customChart();
  }

  render() {
    const post = this.props.data.markdownRemark
    const siteTitle = this.props.data.site.siteMetadata.title
    const { previous, next } = this.props.pageContext
    const image = post.frontmatter.featuredImage? post.frontmatter.featuredImage.childImageSharp.fluid : null
    const comments = this.props.data.comments.edges;

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
        <div className="blog-post" dangerouslySetInnerHTML={{ __html: post.html }} />
        <hr />

        <form name="contact" method="POST" data-netlify="true" data-netlify-honeypot="bot-field">
          <input name="path" type="hidden" value={post.fields.slug} />
          <input type="hidden" name="form-name" value="contact" />
          <p style={{
            marginBottom: rhythm(0.5),
          }}>Leave a comment:</p>
          <div>
            <TextField label="Name" name="Name" style={{
              marginRight: rhythm(0.5),
            }} />
            <TextField label="Email (optional)" input="email" inputMode="email" name="Email" />
          </div>
          <div>
            <TextField label="Message" name="Message" fullWidth multiline />
          </div>
          <div style={{
            marginTop: rhythm(1.0),
          }}>
            <Button variant="outlined" type="submit" color="primary">Send</Button>
          </div>
        </form>
        <hr style={{
              marginTop: rhythm(1.5),
            }} />
        <Bio />

        <div>
        <h4>Comments</h4>

        {comments
          .sort((entryA, entryB) => {
            return new Date(entryA.node.frontmatter.date).valueOf() - new Date(entryB.node.frontmatter.date).valueOf();
          })
          .map(({ node }) => {
            return <CommentCard node={node} key={node.id} />;
          })
        }
        {comments.length === 0 ? (
          <Typography variant="body1" color="textSecondary" component="p">
            There are no comments available for this blog post yet
          </Typography>
        ) : null}
        </div>
          
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
      fields {
        slug
      }
      frontmatter {
        title
        date(formatString: "MMMM DD, YYYY")
        description
        seotitle
        featuredImage {
          childImageSharp {
            fluid(maxWidth: 800) {
              ...GatsbyImageSharpFluid
            }
          }
        }
      }
    }
    comments: allMarkdownRemark(
      filter: {
        fileAbsolutePath: { regex: "/comments/.*/.*\\\\.md$/" }
        frontmatter: { 
          slug: { eq: $slug } 
        }
      }
    ) {
      edges {
        node {
          id
          html
          frontmatter {
            date(formatString: "MMMM DD, YYYY")
            name
          }
        }
      }
    }
  }
`