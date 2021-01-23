import React from 'react';
import { rhythm } from '../utils/typography';
import Card from '@material-ui/core/Card';
import CardContent from '@material-ui/core/CardContent';
import Typography from '@material-ui/core/Typography';
import styled from 'styled-components';

class CommentCard extends React.Component {
  render() {
    const { node } = this.props;
    return (
      <div style={{ marginBottom: rhythm(1) }}>
        <Card>
          <CardContent>
            <Typography variant="h6" component="h2">
              {node.frontmatter.name}
            </Typography>
            <Typography variant="body2" color="textSecondary" component="p">
              {node.frontmatter.date}
            </Typography>
            <StyledComment
              style={{ whiteSpace: 'pre-wrap' }}
              dangerouslySetInnerHTML={{ __html: node.html }}
            />
          </CardContent>
        </Card>
      </div>
    );
  }
}

const StyledComment = styled('div')`
  p {
    margin-bottom: 0;
  }
`;

export default CommentCard;
