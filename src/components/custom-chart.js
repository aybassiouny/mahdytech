import Chart from 'chart.js';

const customChart = function() {
    window.chartColors = {
        red: 'rgb(255, 99, 132)',
        orange: 'rgb(255, 159, 64)',
        yellow: 'rgb(255, 205, 86)',
        green: 'rgb(75, 192, 192)',
        blue: 'rgb(54, 162, 235)',
        purple: 'rgb(153, 102, 255)',
        grey: 'rgb(201, 203, 207)'
      };

    var ctx = document.getElementById('chart1').getContext('2d');
    var chart1 = new Chart(ctx, {
        type: 'line',
        data: {
            labels: ['Avg', '90th', '95th', '99th'],
            datasets: [{
                label: '500 QPS',
                data: [3, 10, 45, 50],
                backgroundColor: window.chartColors.red,
                borderColor: window.chartColors.red,
                borderWidth: 1,
                fill: false,
                lineTension: 0
            },
            {
                label: '1k QPS',
                data: [2, 5, 15, 30],
                backgroundColor: window.chartColors.blue,
                borderColor: window.chartColors.blue,
                borderWidth: 1,
                fill: false,
                lineTension: 0
            },
            {
                label: '30k QPS',
                data: [1, 2, 3, 5],
                backgroundColor: window.chartColors.yellow,
                borderColor: window.chartColors.yellow,
                borderWidth: 1,
                fill: false,
                lineTension: 0
            }]
        },
        options: {
            scales: {
                yAxes: [{
                    ticks: {
                        beginAtZero: true
                    }
                }]
            }
        }
    });
}

export default customChart;