import Chart from 'chart.js';

const customChart = function() {
    const chartColors = {
        red: 'rgb(255, 99, 132)',
        orange: 'rgb(255, 159, 64)',
        yellow: 'rgb(255, 205, 86)',
        green: 'rgb(75, 192, 192)',
        blue: 'rgb(54, 162, 235)',
        purple: 'rgb(153, 102, 255)',
        grey: 'rgb(201, 203, 207)'
    };

    const commonOptions = {
        borderWidth: 1,
        fill: false,
        lineTension: 0
    }

    const colors = ['red', 'blue', 'green','orange', 'purple', 'yellow',  'grey'];

    document.querySelectorAll('.js-chart').forEach(el => {
        var chartData = JSON.parse(el.dataset.chart.replace(/'/g, '"'));
        
        let datasetFromMD = chartData.data;

        for (let i = 0; i < datasetFromMD.length; i++) {
            datasetFromMD[i] = Object.assign({}, commonOptions, datasetFromMD[i]);
            datasetFromMD[i].backgroundColor = chartColors[colors[i]];
            datasetFromMD[i].borderColor = chartColors[colors[i]];
        }

        new Chart(el, {
            type: 'line',
            data: {
                labels: ['Avg', '90th', '95th', '99th'],
                datasets: datasetFromMD
                // datasets: [{
                //     label: '500 QPS',
                //     data: [3, 10, 45, 50],
                //     backgroundColor: window.chartColors.red,
                //     borderColor: window.chartColors.red,
                //     borderWidth: 1,
                //     fill: false,
                //     lineTension: 0
                // },
                // {
                //     label: '1k QPS',
                //     data: [2, 5, 15, 30],
                //     backgroundColor: window.chartColors.blue,
                //     borderColor: window.chartColors.blue,
                //     borderWidth: 1,
                //     fill: false,
                //     lineTension: 0
                // },
                // {
                //     label: '30k QPS',
                //     data: [1, 2, 3, 5],
                //     backgroundColor: window.chartColors.yellow,
                //     borderColor: window.chartColors.yellow,
                //     borderWidth: 1,
                //     fill: false,
                //     lineTension: 0
                // }]
            },
            options: {
                scales: {
                    yAxes: [{
                        ticks: {
                            beginAtZero: true
                        }
                    }]
                },
                tooltips: {
					mode: 'index',
					intersect: false,
                },
                hover: {
					mode: 'nearest',
					intersect: true
				},
				scales: {
					xAxes: [{
						display: true,
						scaleLabel: {
							display: true,
							labelString: 'Latency Percentile'
						}
					}],
					yAxes: [{
						display: true,
						scaleLabel: {
							display: true,
							labelString: 'Latency in ms'
						}
					}]
				}
            }
        });
    })
}

export default customChart;