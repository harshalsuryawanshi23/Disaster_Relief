function renderUnionFind(unionFind, zones) {
  const container = document.getElementById('unionFindViz');
  if (!container) return;
  container.innerHTML = '';

  const components = unionFind.components || [];
  if (!components.length) {
    container.innerHTML = '<div class="panel-empty">No components available</div>';
    return;
  }

  const zoneLookup = new Map((zones || []).map(z => [z.id, z.name]));

  const palette = [
    { bg: '#d8f3dc', border: '#52b788', text: '#1b4332', head: '#2d6a4f' },
    { bg: '#fff3cd', border: '#c77b27', text: '#5a3e12', head: '#9b6e2a' },
    { bg: '#fde8e8', border: '#b94040', text: '#5a1f1f', head: '#b94040' },
    { bg: '#e8eaf6', border: '#5c6bc0', text: '#1a237e', head: '#3949ab' },
    { bg: '#f3e8ff', border: '#8e44ad', text: '#4a235a', head: '#7d3c98' },
  ];

  const wrapper = document.createElement('div');
  wrapper.style.cssText = `
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
    gap: 0.65rem;
    padding: 0.75rem 1rem;
    height: 100%;
    align-content: start;
    overflow-y: auto;
  `;

  components.forEach((component, i) => {
    const p = palette[i % palette.length];
    const card = document.createElement('article');
    card.style.cssText = `
      padding: 0.65rem 0.8rem;
      border-radius: 10px;
      background: ${p.bg};
      border: 1px solid ${p.border};
      border-left: 3px solid ${p.head};
      box-shadow: 0 1px 3px rgba(30,20,10,0.07);
    `;

    const names = component
      .map(id => zoneLookup.get(id) || `Zone ${id}`)
      .join(', ');

    card.innerHTML = `
      <div style="
        font-family: 'DM Mono', monospace;
        font-size: 0.6rem;
        letter-spacing: 0.15em;
        text-transform: uppercase;
        color: ${p.head};
        font-weight: 600;
        margin-bottom: 0.3rem;
      ">Component ${i + 1} · ${component.length} zone${component.length !== 1 ? 's' : ''}</div>
      <div style="
        font-family: 'DM Sans', sans-serif;
        font-size: 0.78rem;
        color: ${p.text};
        line-height: 1.45;
      ">${names}</div>
    `;
    wrapper.appendChild(card);
  });

  container.appendChild(wrapper);
}