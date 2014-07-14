%data1 = voltage
%data2 = inputCurrent
%data3 = outputCurrent
%data4 = state
function testPlot(data, time, labels, testName, xMax)
    newFigure = figure();
    [gAxes, g1, g2] = plotyy(time,data(:,1),time,data(:,2));
    hold on;
    plot(time,data(:,3), 'red');
    plot(time,data(:,4), 'black');
    xlim(gAxes(1), [0, xMax]);
    xlim(gAxes(2), [0, xMax]);
    ylim(gAxes(1), [0, 3.6]);
    ylim(gAxes(2), [0, 3.6]);
    ylabel(gAxes(1), labels(1));
    ylabel(gAxes(2), 'Domain Current (mA)');
    xlabel('Time (ms)');
    title(testName);
    hold off;
    return;
end

