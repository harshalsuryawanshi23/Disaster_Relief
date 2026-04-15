function renderSkipList(skipList) {
  const container = document.getElementById('skipListViz');
  if (!container) {
    return;
  }

  container.innerHTML = '';
  const levels = skipList.levels || [];
  if (!levels.length) {
    container.innerHTML = '<div class="panel-empty">Skip list has no nodes</div>';
    return;
  }

  const width = container.clientWidth || 420;
  const height = container.clientHeight || 220;
  const svg = d3
    .select(container)
    .append('svg')
    .attr('viewBox', `0 0 ${width} ${height}`)
    .attr('width', '100%')
    .attr('height', '100%');

  const laneGap = Math.max(52, height / Math.max(levels.length + 1, 2));

  levels.forEach((levelNodes, levelIndex) => {
    const y = 34 + levelIndex * laneGap;

    svg
      .append('line')
      .attr('x1', 26)
      .attr('x2', width - 26)
      .attr('y1', y)
      .attr('y2', y)
      .attr('stroke', '#d9cebf')
      .attr('stroke-width', 2)
      .attr('stroke-dasharray', '6 7');

    svg
      .append('text')
      .attr('x', 26)
      .attr('y', y - 12)
      .attr('fill', '#6f7166')
      .style('font-family', '"IBM Plex Mono", monospace')
      .style('font-size', '11px')
      .text(`Level ${levels.length - levelIndex - 1}`);

    const gap = Math.max(72, (width - 100) / Math.max(levelNodes.length, 1));
    levelNodes.forEach((node, nodeIndex) => {
      const x = 70 + nodeIndex * gap;

      svg
        .append('rect')
        .attr('x', x - 28)
        .attr('y', y - 18)
        .attr('width', 56)
        .attr('height', 36)
        .attr('rx', 12)
        .attr('fill', '#fffaf3')
        .attr('stroke', '#1d2a22')
        .attr('stroke-width', 2);

      svg
        .append('text')
        .attr('x', x)
        .attr('y', y - 2)
        .attr('text-anchor', 'middle')
        .attr('fill', '#1d2a22')
        .style('font-family', '"IBM Plex Mono", monospace')
        .style('font-size', '11px')
        .text(node.key);

      svg
        .append('text')
        .attr('x', x)
        .attr('y', y + 12)
        .attr('text-anchor', 'middle')
        .attr('fill', '#bf5c2b')
        .style('font-family', '"IBM Plex Mono", monospace')
        .style('font-size', '10px')
        .text(`D${node.depot_id}`);
    });
  });
}
