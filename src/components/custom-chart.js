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
        // borderWidth: 1,
        fill: false,
        lineTension: 0
    }

    const colors = ['red', 'blue', 'green','orange', 'purple', 'yellow',  'grey'];

    document.querySelectorAll('.js-chart').forEach(el => {
        var chartData = JSON.parse(el.dataset.chart.replace(/'/g, '"'));
        let datasetFromMD = chartData.data;
        var chartLabels = el.dataset.labelsChart.split(',');

        for (let i = 0; i < datasetFromMD.length; i++) {
            datasetFromMD[i] = Object.assign({}, commonOptions, datasetFromMD[i]);
            datasetFromMD[i].backgroundColor = chartColors[colors[i]];
            datasetFromMD[i].borderColor = chartColors[colors[i]];
        }

        let yaxisType = el.dataset.yaxisChart? el.dataset.yaxisChart  : 'linear'; 
        let yaxisConfig = [];
        if (yaxisType !== 'logarithmic') 
        {
            yaxisConfig = [{
                display: true,
                type: yaxisType,
                beginAtZero: true,
                scaleLabel: {
                    display: true,
                    labelString: el.dataset.yaxisName
                },
            }]
        }
        else
        {
            let afterBuildTicksFunc = function (chartObj) { //Build ticks labelling as per your need
                chartObj.ticks = [];
                chartObj.ticks.push(1);
                chartObj.ticks.push(10);
                chartObj.ticks.push(100);
                chartObj.ticks.push(1000);
                chartObj.ticks.push(10000);
                chartObj.ticks.push(100000);
                chartObj.ticks.push(1000000);
                chartObj.ticks.push(10000000);
            };
    
            let htmlTicks = {
                min: 1, //minimum tick
                max: 10000000, //maximum tick
                callback: function (value, index, values) {
                    return Number(value.toString());//pass tick values as a string into Number function
                }
            };

            yaxisConfig = [{
                display: true,
                type: yaxisType,
                beginAtZero: true,
                scaleLabel: {
                    display: true,
                    labelString: el.dataset.yaxisName
                },
                ticks: htmlTicks,
                afterBuildTicks: afterBuildTicksFunc
            }];
        }

        new Chart(el, {
            type: 'line',
            data: {
                labels: chartLabels,
                datasets: datasetFromMD
            },
            options: {
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
							labelString: el.dataset.xaxisName
						}
					}],
					yAxes: yaxisConfig
				}
            }
        });
    })
}

export default customChart;