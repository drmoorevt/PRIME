%usage:
%tightPlot3(name,chans,time,avgData(:,:,1,1),avgData(:,:,1,2),avgData(:,:,1,3),50)
function tightPlot3(name,chans,time,data1,data2,data3,xMax)
    hFig = figure(1);
    set(hFig, 'Position', [4000 100 637.5 825])    

    subtightplot(3,1,1,[0.05 0.00], [0.06 0.03], [0.08 0.08]);
    %plot(time,data(:,[1,3,4],1,1));
    [gAxes, g1, g2] = plotyy(time,data1(:,1),time,data1(:,3)); %(outCurrent)
    hold on;
    set(figure(1),{'Color'},{'w'})
    set(g2, {'Color'}, {'r'})
    set(gAxes,{'ycolor'},{'b';'r'})
%    set(gAxes,'Position',[.13,.8,.775,.21])
    plot(time,data1(:,4), 'black'); %(state)
    xlim(gAxes(1), [0, xMax]);
    xlim(gAxes(2), [0, xMax]);
    ylim(gAxes(1), [0, 3.6]);
    ylim(gAxes(2), [0, 3.6]);
    ylabel(gAxes(1), chans(1));
    ylabel(gAxes(2), 'Domain Current (mA)');
    t1 = title(name(1));
    set(t1,{'FontSize'},{10.0})
    
     subtightplot(3,1,2,[0.05 0.00], [0.06 0.03], [0.08 0.08]);
     [gAxes, g1, g2] = plotyy(time,data2(:,1),time,data2(:,3)); %(outCurrent)
     hold on;
     set(g2, {'Color'}, {'r'})
     set(gAxes,{'ycolor'},{'b';'r'})
     plot(time,data2(:,4), 'black'); %(state)
     xlim(gAxes(1), [0, xMax]);
     xlim(gAxes(2), [0, xMax]);
     ylim(gAxes(1), [0, 3.6]);
     ylim(gAxes(2), [0, 3.6]);
     ylabel(gAxes(1), chans(1));
     ylabel(gAxes(2), 'Domain Current (mA)');
     t2 = title(name(2));
     set(t2,{'FontSize'},{10.0})
    
    subtightplot(3,1,3,[0.05 0.00], [0.06 0.03], [0.08 0.08]);
    [gAxes, g1, g2] = plotyy(time,data3(:,1),time,data3(:,3)); %(outCurrent)
    hold on;
    set(g2, {'Color'}, {'r'})
    set(gAxes,{'ycolor'},{'b';'r'})
    plot(time,data3(:,4), 'black'); %(state)
    xlim(gAxes(1), [0, xMax]);
    xlim(gAxes(2), [0, xMax]);
    ylim(gAxes(1), [0, 3.6]);
    ylim(gAxes(2), [0, 3.6]);
    ylabel(gAxes(1), chans(1));
    ylabel(gAxes(2), 'Domain Current (mA)');
    t3 = title(name(3));
    set(t3,{'FontSize'},{10.0})
    xlabel('Time (ms)');
    legend(chans([1,4,3]),'Location','SouthOutside','Orientation','horizontal')
    
    hold off
end