function [indexOfChange] = findStateDemarcation(data, startIndex, range)
  numElem  = numel(data);
  startVal = data(startIndex);
  while (startIndex < numElem)
      if ((startVal >= data(startIndex) + range) || ...
          (startVal <= data(startIndex) - range))
          break;
      else
        startIndex = startIndex + 1;
      end
  end
  indexOfChange = startIndex;
end

