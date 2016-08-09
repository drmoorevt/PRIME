%data1 = voltage
%data2 = inputCurrent
%data3 = outputCurrent
%data4 = state
function testPlot(data, time, labels, testName, xMax)
    newFigure = figure();
    [gAxes, g1, g2] = plotyy(time,data(:,1),time,data(:,3)); %(outCurrent)
    hold on;
    set(newFigure,{'Color'},{'w'})
    set(g2, {'Color'}, {'r'})
    set(gAxes,{'ycolor'},{'b';'r'})
%    set(gAxes,'Position',[.13,.8,.775,.21])
    plot(time,data(:,4), 'black', 'LineWidth', 2); %(state)
    xlim(gAxes(1), [0, xMax]);
    xlim(gAxes(2), [0, xMax]);
    ylim(gAxes(1), [0, 3.6]);
    %ylim(gAxes(2), [0, 50]);
    ylabel(gAxes(1), labels(1));
    ylabel(gAxes(2), 'Domain Current (mA)');
    t1 = title(testName(1));
    set(t1,{'FontSize'},{10.0})
    set(gAxes(1), 'ytick', [0; .5; 4]);
    set(gAxes(2), 'ytick', [0; .5; 4]);
    set(gAxes(1), 'YTickMode', 'auto');
    set(gAxes(2), 'YTickMode', 'auto');
    
%     [gAxes, g1, g2] = plotyy(time,data(:,1),time,data(:,2)); %(inCurrent)
%     hold on;
%     plot(time,data(:,2), 'red'); %(outCurrent)
%     plot(time,data(:,4), 'black'); %(state)
%     xlim(gAxes(1), [0, xMax]);
%     xlim(gAxes(2), [0, xMax]);
%     ylim(gAxes(1), [0, 3.6]);
%     ylim(gAxes(2), [0, 50]);
%     ylabel(gAxes(1), labels(1));
%     ylabel(gAxes(2), 'Domain Current (mA)');
%     xlabel('Time (ms)');
%     title(testName);
%     hold off;
    return;
end

