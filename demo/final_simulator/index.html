<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Simulator: ECS vs CRG (Structural Immunity)</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #1e1e1e; color: #fff; margin: 0; padding: 20px; }
        .container { max-width: 1000px; margin: 0 auto; background: #2d2d2d; padding: 20px; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
        h1 { text-align: center; color: #4da6ff; font-weight: 300; }
        .controls-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 20px; }
        .control-panel { background: #3d3d3d; padding: 15px; border-radius: 8px; display: flex; flex-direction: column; }
        .slider-header { display: flex; justify-content: space-between; margin-bottom: 10px; }
        input[type=range] { width: 100%; cursor: pointer; }
        .metrics { display: flex; justify-content: space-between; margin-bottom: 20px; gap: 20px; }
        .metric-card { background: #3d3d3d; padding: 15px; border-radius: 8px; flex: 1; text-align: center; }
        .metric-value { font-size: 24px; font-weight: bold; margin-top: 10px; }
        .ecs-color { color: #ff4d4d; }
        .crg-color { color: #4dff4d; }
        canvas { background: #1e1e1e; border-radius: 8px; padding: 10px; }
        .explanation { text-align: center; margin-top: 15px; font-style: italic; color: #aaa; font-size: 0.9em; line-height: 1.4; }
    </style>
</head>
<body>

<div class="container">
    <h1>Stability Benchmark: Classic ECS vs CRG</h1>
    
    <div class="controls-grid">
        <div class="control-panel">
            <div class="slider-header">
                <label for="entitySlider"><strong>Total Entities (Volume):</strong></label>
                <span id="entityValue" style="font-weight: bold; color: #4da6ff;">50,000</span>
            </div>
            <input type="range" id="entitySlider" min="10000" max="150000" step="10000" value="50000">
            <small style="color: #aaa; margin-top: 5px;">Scales base iteration cost (ns / entity)</small>
        </div>

        <div class="control-panel">
            <div class="slider-header">
                <label for="mutationSlider"><strong>State Mutations (Chaos):</strong></label>
                <span id="mutationValue" style="font-weight: bold; color: #ffb34d;">0 / frame</span>
            </div>
            <input type="range" id="mutationSlider" min="0" max="50000" step="1000" value="0">
            <small style="color: #aaa; margin-top: 5px;">Triggers structural changes (memcpy overhead)</small>
        </div>
    </div>

    <div class="metrics">
        <div class="metric-card">
            <h3>Classic ECS <br><small style="font-weight: normal; color: #aaa;">(Fast iteration, heavy mutation tax)</small></h3>
            <div class="metric-value ecs-color" id="ecsStatus">1.6 ms</div>
        </div>
        <div class="metric-card">
            <h3>CRG Architecture <br><small style="font-weight: normal; color: #aaa;">(Fixed routing tax, full immunity)</small></h3>
            <div class="metric-value crg-color" id="crgStatus">1.8 ms</div>
        </div>
    </div>

    <canvas id="performanceChart" height="100"></canvas>
    
    <div class="explanation">
        <strong>The "Archetype Poisoning" effect:</strong> At zero mutations, pure ECS iteration is slightly faster due to raw array traversal. 
        CRG pays a fixed routing tax of a few nanoseconds per entity. However, introducing state changes (mutations) causes heavy <code>memcpy</code> overhead and cache misses in ECS. CRG absorbs mutations as simple O(1) state assignments.
    </div>
</div>

<script>
    const entitySlider = document.getElementById('entitySlider');
    const mutationSlider = document.getElementById('mutationSlider');
    const entityDisplay = document.getElementById('entityValue');
    const mutationDisplay = document.getElementById('mutationValue');
    const ecsStatus = document.getElementById('ecsStatus');
    const crgStatus = document.getElementById('crgStatus');

    const ctx = document.getElementById('performanceChart').getContext('2d');
    const maxDataPoints = 50;
    
    const chart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: Array(maxDataPoints).fill(''),
            datasets: [
                {
                    label: 'ECS Frame Time (ms)',
                    borderColor: '#ff4d4d',
                    backgroundColor: 'rgba(255, 77, 77, 0.1)',
                    borderWidth: 2,
                    data: Array(maxDataPoints).fill(1.6),
                    fill: true,
                    tension: 0.1,
                    pointRadius: 0
                },
                {
                    label: 'CRG Frame Time (ms)',
                    borderColor: '#4dff4d',
                    backgroundColor: 'rgba(77, 255, 77, 0.1)',
                    borderWidth: 3,
                    data: Array(maxDataPoints).fill(1.8),
                    fill: true,
                    tension: 0.1,
                    pointRadius: 0
                }
            ]
        },
        options: {
            animation: false,
            responsive: true,
            scales: {
                y: {
                    beginAtZero: true,
                    max: 40,
                    title: { display: true, text: 'Milliseconds (ms)', color: '#fff' },
                    ticks: { color: '#aaa' }
                },
                x: { display: false }
            },
            plugins: { legend: { labels: { color: '#fff' } } }
        },
        plugins: [{
            id: 'horizontalLine',
            beforeDraw: function(chart) {
                const yAxis = chart.scales.y;
                if (!yAxis) return; 
                
                const ctx = chart.ctx;
                const y16 = yAxis.getPixelForValue(16.6);
                
                ctx.save();
                ctx.beginPath();
                ctx.moveTo(chart.chartArea.left, y16);
                ctx.lineTo(chart.chartArea.right, y16);
                ctx.lineWidth = 1;
                ctx.strokeStyle = 'rgba(255, 77, 77, 0.8)';
                ctx.setLineDash([5, 5]);
                ctx.stroke();
                ctx.fillStyle = 'rgba(255, 77, 77, 0.9)';
                ctx.fillText('60 FPS Deadline (16.6ms)', chart.chartArea.left + 10, y16 - 5);
                ctx.restore();
            }
        }]
    });

    entitySlider.addEventListener('input', () => {
        if(parseInt(mutationSlider.value) > parseInt(entitySlider.value)) {
            mutationSlider.value = entitySlider.value;
        }
    });

    // --- HARDWARE PHYSICS MODEL ---
    // Costs in milliseconds (1 ms = 1,000,000 ns)
    const BASE_SYSTEM_OVERHEAD_MS = 0.5; // OS, framework loop...
    
    const GAME_LOGIC_NS = 20; // Simulated work per entity
    const ECS_ITERATION_NS = 3; // Raw array step
    const CRG_ROUTING_NS = 8; // Array step + static VTable / if constexpr

    const ECS_COST_PER_ENTITY_MS = (GAME_LOGIC_NS + ECS_ITERATION_NS) / 1000000;
    const CRG_COST_PER_ENTITY_MS = (GAME_LOGIC_NS + CRG_ROUTING_NS) / 1000000;

    const ECS_ARCHETYPE_MOVE_MS = 0.0012; // ~1200ns per mutation (memcpy + structural change)
    const CRG_STATE_ASSIGN_MS = 0.000005; // ~5ns per mutation (simple integer reassignment)

    setInterval(() => {
        if (!chart || !chart.data) return;

        const entities = parseInt(entitySlider.value);
        const mutations = parseInt(mutationSlider.value);
        
        entityDisplay.innerText = entities.toLocaleString();
        mutationDisplay.innerText = mutations.toLocaleString() + " / frame";

        // --- Simulator Logic ---
        
        // 1. ECS: Pays entity volume + Heavy mutation tax + Cache miss chaos
        let ecsBaseTime = BASE_SYSTEM_OVERHEAD_MS + (entities * ECS_COST_PER_ENTITY_MS); 
        let ecsTax = mutations * ECS_ARCHETYPE_MOVE_MS; 
        let ecsChaos = mutations > 500 ? (Math.random() * (mutations * 0.0004)) : 0; 
        
        let ecsTotalTime = ecsBaseTime + ecsTax + ecsChaos;
        if(ecsTotalTime > 50) ecsTotalTime = 45 + Math.random() * 5; 

        // 2. CRG: Pays higher entity volume (routing tax), but ignores mutation chaos
        let crgBaseTime = BASE_SYSTEM_OVERHEAD_MS + (entities * CRG_COST_PER_ENTITY_MS);
        let crgTax = mutations * CRG_STATE_ASSIGN_MS; 
        let crgTotalTime = crgBaseTime + crgTax;

        // UI Text Update
        if (ecsTotalTime > 16.6) {
            ecsStatus.innerHTML = `${ecsTotalTime.toFixed(1)} ms <br><small style="color: #ff4d4d; font-size: 0.6em;">Archetype Poisoning</small>`;
            ecsStatus.style.color = '#ff4d4d';
        } else {
            ecsStatus.innerHTML = `${ecsTotalTime.toFixed(1)} ms <br><small style="color: #ffb34d; font-size: 0.6em;">Stable Array</small>`;
            ecsStatus.style.color = '#ffb34d';
        }
        crgStatus.innerHTML = `${crgTotalTime.toFixed(1)} ms <br><small style="color: #4dff4d; font-size: 0.6em;">Constant Time</small>`;

        // Synchronize arrays
        chart.data.labels.shift();
        chart.data.labels.push('');

        chart.data.datasets[0].data.shift();
        chart.data.datasets[0].data.push(ecsTotalTime);
        
        chart.data.datasets[1].data.shift();
        chart.data.datasets[1].data.push(crgTotalTime);
        
        chart.update();

    }, 100); 
</script>

</body>
</html>